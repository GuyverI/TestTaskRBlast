#include "Button.h"

namespace ButtonUtils
{
	template<class T>
	T* Init(T* button)
	{
		if (button && button->init()) {
			button->autorelease();
			return button;
		}

		CC_SAFE_DELETE(button);
		return nullptr;
	}
}


const std::string Button::EVENT_PRESSED = "ButtonPressed";

const std::string Button::EVENT_TAP_BEGIN = "tapBegin";
const std::string Button::EVENT_TAP_END = "tapEnd";
const std::string Button::EVENT_TAP_MOVED_OUT = "tapMovedOut";
const std::string Button::EVENT_TAP_MOVED_IN = "tapMovedIn";
const std::string Button::EVENT_TAP_END_OUT = "tapEndOut";

Button* Button::Create()
{
	return ButtonUtils::Init(new Button());
}

bool Button::init()
{
	cocos2d::Node::init();

	auto touchListener = cocos2d::EventListenerTouchOneByOne::create();
	touchListener->onTouchBegan = CC_CALLBACK_2(Button::OnTouchBegan, this);
	touchListener->onTouchEnded = CC_CALLBACK_2(Button::OnTouchEnded, this);
	touchListener->onTouchMoved = CC_CALLBACK_2(Button::OnTouchMoved, this);
	touchListener->onTouchCancelled = CC_CALLBACK_2(Button::OnTouchCancelled, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

	SetState(STATE_IDLE);

	return true;
}

void Button::setContentSize(const cocos2d::Size& contentSize)
{
	cocos2d::Node::setContentSize(contentSize);
	FixZones();
}

bool Button::IsValidState(const State state) const
{
	return state >= STATE_IDLE && state <= STATE_DRAGOUT;
}

void Button::SetExpandSize(const cocos2d::Size& size)
{
	_expendSize = size;
	FixZones();
}

cocos2d::Rect Button::GetExpandZone() const
{
	return GetZoneRect(_expendSize);
}

void Button::SetSafeSize(const cocos2d::Size& size)
{
	_safeSize = size;
	FixZone(_expendSize, _safeSize);
}

cocos2d::Rect Button::GetSafeZone() const
{
	return GetZoneRect(_safeSize);
}

void Button::AddChildToState(const State state, cocos2d::Node* child)
{
	if (!IsValidState(state)) {
		return;
	}

	_stateChildren.push_back(std::make_pair(state, child));
	this->addChild(child);

	child->setVisible(state == _state);
}

void Button::FixZone(const cocos2d::Size& reference, cocos2d::Size& zoneSize)
{
	if (zoneSize.width < reference.width || zoneSize.height < reference.height) {
		zoneSize = reference;
	}
}

Button::Button()
	: _state(STATE_IDLE)
	, _stateTransitions()
	, _expendSize()
	, _safeSize()
	, _stateChildren()
	, _callback()
{
	_stateTransitions[EVENT_TAP_BEGIN].insert(std::make_pair(STATE_IDLE, STATE_PUSHED));
	_stateTransitions[EVENT_TAP_END].insert(std::make_pair(STATE_PUSHED, STATE_IDLE));
	_stateTransitions[EVENT_TAP_MOVED_OUT].insert(std::make_pair(STATE_PUSHED, STATE_DRAGOUT));
	_stateTransitions[EVENT_TAP_MOVED_IN].insert(std::make_pair(STATE_DRAGOUT, STATE_PUSHED));
	_stateTransitions[EVENT_TAP_END_OUT].insert(std::make_pair(STATE_DRAGOUT, STATE_IDLE));
}

cocos2d::Rect Button::GetZoneRect(const cocos2d::Size& zoneSize) const
{
	cocos2d::Rect rect(-zoneSize.width / 2.f, -zoneSize.height / 2.f, zoneSize.width, zoneSize.height);
	return RectApplyAffineTransform(rect, getNodeToParentAffineTransform());
}

void Button::FixZones()
{
	FixZone(_contentSize, _expendSize);
	FixZone(_expendSize, _safeSize);
}

void Button::UpdateState()
{
	for (auto& node : _stateChildren) {
		node.second->setVisible(node.first == _state);
	}
}

void Button::ProccessEvent(const std::string& event)
{
	if (event == EVENT_TAP_END && _state == STATE_PUSHED) {
		OnTouchSuccess();
	}

	MakeStateTransition(event);
}

void Button::MakeStateTransition(const std::string& event)
{
	const auto& pairSet = _stateTransitions[event];
	for (const auto& pair : pairSet)
	{
		if (pair.first == _state) {
			SetState(pair.second);
		}
	}
}

void Button::SetState(const State state)
{
	if (!IsValidState(state)) {
		return;
	}

	if (_state != state)
	{
		const State prevState = _state;
		_state = state;
		UpdateState();

		OnStateChanged(prevState, _state);
	}
}

bool Button::OnTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event)
{
	if (GetExpandZone().containsPoint(touch->getLocation()))
	{
		ProccessEvent(EVENT_TAP_BEGIN);
		return true;
	}

	return false;
}

void Button::OnTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event)
{
	ProccessEvent(EVENT_TAP_END);
}

void Button::OnTouchMoved(cocos2d::Touch* touch, cocos2d::Event* event)
{
	const auto touchPos = touch->getLocation();

	if (GetExpandZone().containsPoint(touchPos)) {
		ProccessEvent(EVENT_TAP_MOVED_IN);
	}
	else if (GetSafeZone().containsPoint(touchPos)) {
		ProccessEvent(EVENT_TAP_MOVED_OUT);
	}
	else {
		ProccessEvent(EVENT_TAP_END_OUT);
	}
}

void Button::OnTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event)
{
	SetState(STATE_IDLE);
}

void Button::OnTouchSuccess()
{
	EventParams params = { this, _state };

	auto event = cocos2d::EventCustom(EVENT_PRESSED);
	event.setUserData(&params);
	_eventDispatcher->dispatchEvent(&event);

	if (_callback) {
		_callback();
	}
}


const std::string LongPushButton::EVENT_TAP_END_LONG = "tapEndLong";
const std::string LongPushButton::EVENT_TIMEOUT = "timeout";

LongPushButton* LongPushButton::Create(const float longPushTimeout)
{
	return ButtonUtils::Init(new LongPushButton(longPushTimeout));
}

bool LongPushButton::IsValidState(const State state) const
{
	return Button::IsValidState(state) || state == STATE_PUSHED_LONG;
}

void LongPushButton::OnStateChanged(const State prevState, const State newState)
{
	Button::OnStateChanged(prevState, newState);

	const auto scheduleKey = std::string("longPushedKey");
	if (newState == STATE_PUSHED && _longPressTimeout >= 0.f)
	{
		this->scheduleOnce([this](float) { ProccessEvent(EVENT_TIMEOUT); }, _longPressTimeout, scheduleKey);
	}
	else if (this->isScheduled(scheduleKey))
	{
		this->unschedule(scheduleKey);
	}
}

void LongPushButton::ProccessEvent(const std::string& event)
{
	if (event == EVENT_TAP_END_LONG && _state == STATE_PUSHED_LONG) {
		OnTouchSuccess();
	}

	Button::ProccessEvent(event);
}

void LongPushButton::OnTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event)
{
	if (_state == STATE_PUSHED_LONG) {
		ProccessEvent(EVENT_TAP_END_LONG);
	}
	else {
		Button::OnTouchEnded(touch, event);
	}
}

LongPushButton::LongPushButton(const float longPushTimeout)
	: Button()
	, _longPressTimeout(longPushTimeout)
{
	_stateTransitions[EVENT_TAP_END_LONG].insert(std::make_pair(STATE_PUSHED_LONG, STATE_IDLE));
	_stateTransitions[EVENT_TIMEOUT].insert(std::make_pair(STATE_PUSHED, STATE_PUSHED_LONG));
	_stateTransitions[EVENT_TAP_MOVED_OUT].insert(std::make_pair(STATE_PUSHED_LONG, STATE_DRAGOUT));
}