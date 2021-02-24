#pragma  once

#include "cocos2d.h"

class Button : public cocos2d::Node
{
public:

	using State = int;
	static const State STATE_IDLE = 0;
	static const State STATE_PUSHED = 1;
	static const State STATE_DRAGOUT = 2;

	static const std::string EVENT_PRESSED;

	struct EventParams
	{
		Button* senderButton;
		State sentFromState;
	};

	using Callback = std::function<void()>;

public:
	static Button* Create();

	virtual bool init() override;
	virtual void setContentSize(const cocos2d::Size& contentSize) override;

	void SetExpandSize(const cocos2d::Size& size);
	cocos2d::Rect GetExpandZone() const;

	void SetSafeSize(const cocos2d::Size& size);
	cocos2d::Rect GetSafeZone() const;

	void AddChildToState(const State state, cocos2d::Node* child);

	void SetCallback(const Callback& cb) { _callback = cb; }

protected:
	static const std::string EVENT_TAP_BEGIN;
	static const std::string EVENT_TAP_END;
	static const std::string EVENT_TAP_MOVED_OUT;
	static const std::string EVENT_TAP_MOVED_IN;
	static const std::string EVENT_TAP_END_OUT;

	Button();

	virtual bool IsValidState(const State state) const;
	virtual void OnStateChanged(const State prevState, const State newState) {}
	virtual void ProccessEvent(const std::string& event);

	virtual bool OnTouchBegan(cocos2d::Touch*, cocos2d::Event*);
	virtual void OnTouchEnded(cocos2d::Touch*, cocos2d::Event*);
	virtual void OnTouchMoved(cocos2d::Touch*, cocos2d::Event*);
	virtual void OnTouchCancelled(cocos2d::Touch*, cocos2d::Event*);

	void OnTouchSuccess();

protected:
	State _state;
	std::map<std::string, std::set<std::pair<State, State>>> _stateTransitions;

private:
	static void FixZone(const cocos2d::Size& reference, cocos2d::Size& zoneSize);

	cocos2d::Rect GetZoneRect(const cocos2d::Size& zoneSize) const;
	void FixZones();
	void UpdateState();

	void MakeStateTransition(const std::string& event);
	void SetState(const State state);

private:
	cocos2d::Size _expendSize;
	cocos2d::Size _safeSize;
	std::vector<std::pair<State, cocos2d::Node*>> _stateChildren;
	Callback _callback;
};

class LongPushButton : public Button
{
public:
	static const State STATE_PUSHED_LONG = 3;

	static LongPushButton* Create(const float longPushTimeout);

protected:
	static const std::string EVENT_TAP_END_LONG;
	static const std::string EVENT_TIMEOUT;

	virtual bool IsValidState(const State state) const override;
	virtual void OnStateChanged(const State prevState, const State newState) override;
	virtual void ProccessEvent(const std::string& event) override;
	virtual void OnTouchEnded(cocos2d::Touch*, cocos2d::Event*) override;

private:
	explicit LongPushButton(const float longPushTimeout);

	float _longPressTimeout = -1.f;
};