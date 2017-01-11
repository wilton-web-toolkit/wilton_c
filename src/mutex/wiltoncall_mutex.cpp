/* 
 * File:   wiltoncall_mutex.cpp
 * Author: alex
 *
 * Created on January 11, 2017, 9:34 AM
 */

#include "call/wiltoncall_internal.hpp"

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"
#include "staticlib/utils.hpp"

#include "wilton/wilton.h"
#include "wilton/wiltoncall.h"

namespace wilton {
namespace mutex {

namespace { //anonymous

namespace sc = staticlib::config;
namespace si = staticlib::io;
namespace ss = staticlib::serialization;
namespace su = staticlib::utils;

common::handle_registry<wilton_Mutex>& static_registry() {
    static common::handle_registry<wilton_Mutex> registry;
    return registry;
}

// shouldn't be called before logging is initialized by app
void log_error(const std::string& message) {
    static std::string level = "ERROR";
    static std::string logger = "wilton.mutex";
    // call wilton
    wilton_logger_log(level.c_str(), level.length(), logger.c_str(), logger.length(),
            message.c_str(), message.length());
}

} // namespace

std::string mutex_create(const std::string&) {
    wilton_Mutex* mutex;
    char* err = wilton_Mutex_create(std::addressof(mutex));
    if (nullptr != err) common::throw_wilton_error(err, TRACEMSG(std::string(err) +
            "\nmutex_create error"));
    int64_t handle = static_registry().put(mutex);
    return ss::dump_json_to_string({
        { "mutexHandle", handle}
    });
}

std::string mutex_lock(const std::string& data) {
    // json parse
    ss::JsonValue json = ss::load_json_from_string(data);
    int64_t handle = -1;
    for (const ss::JsonField& fi : json.as_object()) {
        auto& name = fi.name();
        if ("mutexHandle" == name) {
            handle = common::get_json_int64(fi);
        } else {
            throw common::WiltonInternalException(TRACEMSG("Unknown data field: [" + name + "]"));
        }
    }
    if (-1 == handle) throw common::WiltonInternalException(TRACEMSG(
            "Required parameter 'mutexHandle' not specified"));
    // get handle
    wilton_Mutex* mutex = static_registry().peek(handle);
    if (nullptr == mutex) throw common::WiltonInternalException(TRACEMSG(
            "Invalid 'mutexHandle' parameter specified"));
    // call wilton
    char* err = wilton_Mutex_lock(mutex);
    if (nullptr != err) {
        common::throw_wilton_error(err, TRACEMSG(err));
    }
    return "{}";
}

std::string mutex_unlock(const std::string& data) {
    // json parse
    ss::JsonValue json = ss::load_json_from_string(data);
    int64_t handle = -1;
    for (const ss::JsonField& fi : json.as_object()) {
        auto& name = fi.name();
        if ("mutexHandle" == name) {
            handle = common::get_json_int64(fi);
        } else {
            throw common::WiltonInternalException(TRACEMSG("Unknown data field: [" + name + "]"));
        }
    }
    if (-1 == handle) throw common::WiltonInternalException(TRACEMSG(
            "Required parameter 'mutexHandle' not specified"));
    // get handle
    wilton_Mutex* mutex = static_registry().peek(handle);
    if (nullptr == mutex) throw common::WiltonInternalException(TRACEMSG(
            "Invalid 'mutexHandle' parameter specified"));
    // call wilton
    char* err = wilton_Mutex_unlock(mutex);
    if (nullptr != err) {
        common::throw_wilton_error(err, TRACEMSG(err));
    }
    return "{}";
}

std::string mutex_wait(const std::string& data) {
    // json parse
    ss::JsonValue json = ss::load_json_from_string(data);
    auto rcallback = std::ref(common::empty_json());
    int64_t handle = -1;
    int64_t timeout_millis = -1;
    for (const ss::JsonField& fi : json.as_object()) {
        auto& name = fi.name();
        if ("conditionCallbackScript" == name) {
            rcallback = fi.value();
        } else if ("mutexHandle" == name) {
            handle = common::get_json_int64(fi);
        } else if ("timeoutMillis" == name) {
            timeout_millis = common::get_json_int64(fi);
        } else {
            throw common::WiltonInternalException(TRACEMSG("Unknown data field: [" + name + "]"));
        }
    }
    if (ss::JsonType::NULL_T == rcallback.get().type()) throw common::WiltonInternalException(TRACEMSG(
            "Required parameter 'conditionCallbackScript' not specified"));
    if (-1 == handle) throw common::WiltonInternalException(TRACEMSG(
            "Required parameter 'mutexHandle' not specified"));
    if (-1 == timeout_millis) throw common::WiltonInternalException(TRACEMSG(
            "Required parameter 'timeoutMillis' not specified"));
    const ss::JsonValue& callback = rcallback.get();
    std::string* callback_str_ptr = new std::string();
    *callback_str_ptr = ss::dump_json_to_string(callback);
    // get handle
    wilton_Mutex* mutex = static_registry().peek(handle);
    if (nullptr == mutex) throw common::WiltonInternalException(TRACEMSG(
            "Invalid 'mutexHandle' parameter specified"));
    // call wilton
    char* err = wilton_Mutex_wait(mutex, static_cast<int> (timeout_millis), callback_str_ptr,
            [](void* passed) {
                std::string* sptr = static_cast<std::string*> (passed);
                char* out;
                int out_len;
                auto err = wiltoncall_runscript(sptr->c_str(), static_cast<int> (sptr->length()),
                        std::addressof(out), std::addressof(out_len));
                delete sptr;
                if (nullptr != err) {
                    log_error(TRACEMSG(err));
                    wilton_free(err);
                    return 1;
                }
                if (nullptr == out) {
                    log_error(TRACEMSG("Null condition result returned"));
                    return 1;
                }
                if (!su::is_positive_uint16(out_len)) {
                    log_error(TRACEMSG(
                        "Invalid output length returned from condition: [" + sc::to_string(out_len) + "]"));
                    return 1;
                }
                // parse json
                auto src = si::array_source(out, static_cast<uint16_t> (out_len));
                ss::JsonValue res = ss::load_json(src);
                int32_t tribool = -1;
                for (const ss::JsonField& fi : res.as_object()) {
                    auto& name = fi.name();
                    if ("condition" == name && ss::JsonType::BOOLEAN == fi.type()) {
                        tribool = fi.as_bool() ? 1 : 0;
                    } else {
                        log_error(TRACEMSG("Unknown condition result data field: [" + name + "]"));
                        return 1;
                    }
                }
                if (-1 == tribool) {
                    std::string res_str{out, static_cast<uint16_t> (out_len)};
                    log_error(TRACEMSG("Required parameter 'condition' not specified" +
                            " in condition result: [" + res_str + "]"));
                }
                bool cond_res = 1 == tribool;
                return cond_res ? 1 : 0;
            });
    if (nullptr != err) {
        common::throw_wilton_error(err, TRACEMSG(err));
    }
    return "{}";
}

std::string mutex_notify_all(const std::string& data) {
    // json parse
    ss::JsonValue json = ss::load_json_from_string(data);
    int64_t handle = -1;
    for (const ss::JsonField& fi : json.as_object()) {
        auto& name = fi.name();
        if ("mutexHandle" == name) {
            handle = common::get_json_int64(fi);
        } else {
            throw common::WiltonInternalException(TRACEMSG("Unknown data field: [" + name + "]"));
        }
    }
    if (-1 == handle) throw common::WiltonInternalException(TRACEMSG(
            "Required parameter 'mutexHandle' not specified"));
    // get handle
    wilton_Mutex* mutex = static_registry().peek(handle);
    if (nullptr == mutex) throw common::WiltonInternalException(TRACEMSG(
            "Invalid 'mutexHandle' parameter specified"));
    // call wilton
    char* err = wilton_Mutex_notify_all(mutex);
    if (nullptr != err) {
        common::throw_wilton_error(err, TRACEMSG(err));
    }
    return "{}";
}

std::string mutex_destroy(const std::string& data) {
    // json parse
    ss::JsonValue json = ss::load_json_from_string(data);
    int64_t handle = -1;
    for (const ss::JsonField& fi : json.as_object()) {
        auto& name = fi.name();
        if ("mutexHandle" == name) {
            handle = common::get_json_int64(fi);
        } else {
            throw common::WiltonInternalException(TRACEMSG("Unknown data field: [" + name + "]"));
        }
    }
    if (-1 == handle) throw common::WiltonInternalException(TRACEMSG(
            "Required parameter 'mutexHandle' not specified"));
    // get handle
    wilton_Mutex* mutex = static_registry().remove(handle);
    if (nullptr == mutex) throw common::WiltonInternalException(TRACEMSG(
            "Invalid 'mutexHandle' parameter specified"));
    // call wilton
    char* err = wilton_Mutex_destroy(mutex);
    if (nullptr != err) {
        static_registry().put(mutex);
        common::throw_wilton_error(err, TRACEMSG(err));
    }
    return "{}";
}


} // namespace
}
