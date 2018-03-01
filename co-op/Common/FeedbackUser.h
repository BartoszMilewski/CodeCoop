#if !defined (FEEDBACKUSER_H)
#define FEEDBACKUSER_H
//------------------------------------
// (c) Reliable Software 2002 -- 2002
//------------------------------------

class FeedbackManager;

class FeedbackUser
{
public:
	virtual ~FeedbackUser () {}

	virtual FeedbackManager * SetUIFeedback (FeedbackManager * feedback) = 0;
};

class FeedbackHolder
{
public:
	FeedbackHolder(FeedbackUser & feedbackUser, FeedbackManager * feedback)
	: _feedbackUser(feedbackUser),
	  _prevFeedbackManager(_feedbackUser.SetUIFeedback(feedback))
	{
	}

	~FeedbackHolder()
	{
		_feedbackUser.SetUIFeedback(_prevFeedbackManager);
	}

private:
	FeedbackUser & _feedbackUser;
	FeedbackManager * const _prevFeedbackManager;
};

#endif

