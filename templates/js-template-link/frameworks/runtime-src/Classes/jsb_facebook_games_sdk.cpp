#include "scripting/js-bindings/manual/jsb_conversions.hpp"
#include "scripting/js-bindings/manual/jsb_global.h"

#include "Event.h"
#include "FacebookGameSDK.h"
#include "SystemDefaultBrowser.h"

#include "jsb_facebook_games_sdk.hpp"

#include <locale>
#include <codecvt>

NS_CC_BEGIN

static const utility::string_t kExternId(U("<extern_id_here>"));
static const utility::string_t kExternNamespace(U("<extern_namespace_here>"));

// The Facebook Games SDK wrapper
class FacebookPCGamesSDK : public Ref
{
public:
	FacebookPCGamesSDK()
		: appId(0)
		, accessToken(U(""))
		, _isEventPrepared(false)
		, loginUser(nullptr)
	{
	}

	~FacebookPCGamesSDK() {
		loginUser.reset();
	}


	// param  Facebook Developer Account ID
	void init(std::string strAppId)
	{
		appId = std::stoull(strAppId);
		const auto browser = std::make_shared<facebook_games_sdk::SystemDefaultBrowser>(appId);
		facebook_games_sdk::FacebookGameSDK::initialize(appId, browser);
	}

	std::shared_ptr<facebook_games_sdk::User> login()
	{
		auto user = facebook_games_sdk::FacebookGameSDK::getInstance().doLogin().then([=](std::shared_ptr<facebook_games_sdk::User> usr) {
			if (usr) {
				usr->getPermissions().get();
			}
			return usr;
		});
		// Calling future::get on a valid future blocks the thread until the provider makes the shared state ready
		loginUser = user.get();

		return loginUser;
	}

	// push event to Analysis Statistics
	void logEvent(const utility::string_t& event_name,
		const std::map<utility::string_t, utility::string_t>& details = {},
		const double value_to_sum = 0.0)
	{
		_isEventPrepared ? nullptr : _prepareEvent();

		facebook_games_sdk::Event::logEvent(event_name, details, value_to_sum);
	}

private:

	void _prepareEvent()
	{
		accessToken = loginUser ? loginUser->accessToken() : U("");
		// To send events to Facebook
		// You would be able to see the events in Analytics
		facebook_games_sdk::Event::initialize(appId, accessToken);

		// Add additional information about the user
		facebook_games_sdk::Event::setExternalInfo(kExternId, kExternNamespace);

		_isEventPrepared = true;
	}

private:

	unsigned long long appId;

	std::wstring accessToken;

	std::shared_ptr<facebook_games_sdk::User> loginUser;

	bool _isEventPrepared;
};

NS_CC_END

// conversions
bool FB_User_to_seval(std::shared_ptr<facebook_games_sdk::User> user, se::Value* ret)
{
	assert(ret != nullptr);
	std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

	se::HandleObject obj(se::Object::createPlainObject());
	obj->setProperty("email", se::Value(convert.to_bytes(user->email())));
	obj->setProperty("name", se::Value(convert.to_bytes(user->name())));
	obj->setProperty("accessToken", se::Value(convert.to_bytes(user->accessToken())));
	obj->setProperty("hasPicture", se::Value(user->hasPicture()));
	ret->setObject(obj);

	return true;
};

bool seval_to_FB_Strings(const se::Value& v, std::wstring* ret)
{
	assert(ret != nullptr);
	std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

	*ret = convert.from_bytes(v.toStringForce());
	return true;
};

bool seval_to_FB_StringsMap(const se::Value& v, std::map<utility::string_t, utility::string_t>* ret)
{
	assert(ret != nullptr);
	std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

	std::map<std::string, std::string> tmp;
	seval_to_std_map_string_string(v, &tmp);
	for (auto iter : tmp)
	{
		(*ret)[convert.from_bytes(iter.first)] = convert.from_bytes(iter.second);
	}

	return true;
};

se::Object* __jsb_cocos2d_FacebookPCGamesSDK_proto = nullptr;
se::Class* __jsb_cocos2d_FacebookPCGamesSDK_class = nullptr;

static bool js_facebook_pc_games_sdk_FacebookPCGamesSDK_init(se::State& s)
{
	cocos2d::FacebookPCGamesSDK* cobj = (cocos2d::FacebookPCGamesSDK*)s.nativeThisObject();
	SE_PRECONDITION2(cobj, false, "js_facebook_pc_games_sdk_FacebookPCGamesSDK_init : Invalid Native Object");
	const auto& args = s.args();
	size_t argc = args.size();
	CC_UNUSED bool ok = true;
	if (argc == 1) {
		std::string arg0;
		ok &= seval_to_std_string(args[0], &arg0);
		SE_PRECONDITION2(ok, false, "js_facebook_pc_games_sdk_FacebookPCGamesSDK_init : Error processing arguments");
		cobj->init(arg0);
		return true;
	}
	SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 1);
	return false;
}
SE_BIND_FUNC(js_facebook_pc_games_sdk_FacebookPCGamesSDK_init)

static bool js_facebook_pc_games_sdk_FacebookPCGamesSDK_logEvent(se::State& s)
{
	cocos2d::FacebookPCGamesSDK* cobj = (cocos2d::FacebookPCGamesSDK*)s.nativeThisObject();
	SE_PRECONDITION2(cobj, false, "js_facebook_pc_games_sdk_FacebookPCGamesSDK_logEvent : Invalid Native Object");
	const auto& args = s.args();
	size_t argc = args.size();
	CC_UNUSED bool ok = true;
	if (argc == 1) {
		std::wstring event_name;
		ok &= seval_to_FB_Strings(args[0], &event_name);
		SE_PRECONDITION2(ok, false, "Error processing arguments");
		cobj->logEvent(event_name);
		return true;
	}
	else if (argc == 2)
	{
		std::wstring event_name;
		ok &= seval_to_FB_Strings(args[0], &event_name);
		std::map<utility::string_t, utility::string_t> stringsMap;
		ok &= seval_to_FB_StringsMap(args[1], &stringsMap);
		SE_PRECONDITION2(ok, false, "Error processing arguments");
		cobj->logEvent(event_name, stringsMap);
		return true;
	}
	else if (argc == 3)
	{
		std::wstring event_name;
		ok &= seval_to_FB_Strings(args[0], &event_name);
		std::map<utility::string_t, utility::string_t> stringsMap;
		ok &= seval_to_FB_StringsMap(args[1], &stringsMap);
		double value_to_sum;
		ok &= seval_to_double(args[2], &value_to_sum);
		SE_PRECONDITION2(ok, false, "Error processing arguments");
		cobj->logEvent(event_name, stringsMap, value_to_sum);
		return true;
	}
	SE_REPORT_ERROR("wrong number of arguments: %d", (int)argc);
	return false;
}
SE_BIND_FUNC(js_facebook_pc_games_sdk_FacebookPCGamesSDK_logEvent)

static bool js_facebook_pc_games_sdk_FacebookPCGamesSDK_login(se::State& s)
{
	cocos2d::FacebookPCGamesSDK* cobj = (cocos2d::FacebookPCGamesSDK*)s.nativeThisObject();
	SE_PRECONDITION2(cobj, false, "js_facebook_pc_games_sdk_FacebookPCGamesSDK_login : Invalid Native Object");
	const auto& args = s.args();
	size_t argc = args.size();
	if (argc == 0) {
		std::shared_ptr<facebook_games_sdk::User> user = cobj->login();
		bool ok = FB_User_to_seval(user, &s.rval());
		SE_PRECONDITION2(ok, false, "js_facebook_pc_games_sdk_FacebookPCGamesSDK_login : FB_User_to_seval failed");
		return true;
	}
	SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
	return false;
}
SE_BIND_FUNC(js_facebook_pc_games_sdk_FacebookPCGamesSDK_login)

SE_DECLARE_FINALIZE_FUNC(js_cocos2d_FacebookPCGamesSDK_finalize)

static bool js_facebook_pc_games_sdk_FacebookPCGamesSDK_constructor(se::State& s)
{
	cocos2d::FacebookPCGamesSDK* cobj = new (std::nothrow) cocos2d::FacebookPCGamesSDK();
	s.thisObject()->setPrivateData(cobj);
	return true;
}
SE_BIND_CTOR(js_facebook_pc_games_sdk_FacebookPCGamesSDK_constructor, __jsb_cocos2d_FacebookPCGamesSDK_class, js_cocos2d_FacebookPCGamesSDK_finalize)



static bool js_cocos2d_FacebookPCGamesSDK_finalize(se::State& s)
{
	CCLOGINFO("jsbindings: finalizing JS object %p (cocos2d::FacebookPCGamesSDK)", s.nativeThisObject());
	cocos2d::FacebookPCGamesSDK* cobj = (cocos2d::FacebookPCGamesSDK*)s.nativeThisObject();
	cobj->release();
	return true;
}
SE_BIND_FINALIZE_FUNC(js_cocos2d_FacebookPCGamesSDK_finalize)

bool js_register_facebook_pc_games_sdk_FacebookPCGamesSDK(se::Object* obj)
{
	auto cls = se::Class::create("FacebookPCGamesSDK", obj, nullptr, _SE(js_facebook_pc_games_sdk_FacebookPCGamesSDK_constructor));

	cls->defineFunction("init", _SE(js_facebook_pc_games_sdk_FacebookPCGamesSDK_init));
	cls->defineFunction("logEvent", _SE(js_facebook_pc_games_sdk_FacebookPCGamesSDK_logEvent));
	cls->defineFunction("login", _SE(js_facebook_pc_games_sdk_FacebookPCGamesSDK_login));
	cls->defineFinalizeFunction(_SE(js_cocos2d_FacebookPCGamesSDK_finalize));
	cls->install();
	JSBClassType::registerClass<cocos2d::FacebookPCGamesSDK>(cls);

	__jsb_cocos2d_FacebookPCGamesSDK_proto = cls->getProto();
	__jsb_cocos2d_FacebookPCGamesSDK_class = cls;

	se::ScriptEngine::getInstance()->clearException();
	return true;
}

bool register_all_facebook_pc_games_sdk(se::Object* obj)
{
	// Get the ns
	se::Value nsVal;
	if (!obj->getProperty("jsb", &nsVal))
	{
		se::HandleObject jsobj(se::Object::createPlainObject());
		nsVal.setObject(jsobj);
		obj->setProperty("jsb", nsVal);
	}
	se::Object* ns = nsVal.toObject();

	js_register_facebook_pc_games_sdk_FacebookPCGamesSDK(ns);
	return true;
}

