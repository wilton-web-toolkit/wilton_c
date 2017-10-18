/* 
 * File:   wiltoncall_internal.hpp
 * Author: alex
 *
 * Created on January 10, 2017, 11:42 AM
 */

#ifndef WILTON_CALL_WILTONCALL_INTERNAL_HPP
#define WILTON_CALL_WILTONCALL_INTERNAL_HPP

#include <cstdint>
#include <thread>
#include <string>

#include "staticlib/json.hpp"
#include "staticlib/io.hpp"
#include "staticlib/support.hpp"
#include "staticlib/unzip.hpp"
#include "staticlib/utils.hpp"

#include "wilton/wiltoncall.h"

#include "wilton/support/buffer.hpp"
#include "wilton/support/exception.hpp"
#include "wilton/support/handle_registry.hpp"
#include "wilton/support/payload_handle_registry.hpp"

namespace wilton {

// Logger
namespace logging {

support::buffer logging_initialize(sl::io::span<const char> data);

support::buffer logging_log(sl::io::span<const char> data);

support::buffer logging_is_level_enabled(sl::io::span<const char> data);

support::buffer logging_shutdown(sl::io::span<const char> data);

} // namespace

// load

namespace load {

support::buffer load_module_resource(sl::io::span<const char> data);

support::buffer load_module_script(sl::io::span<const char> data);

}

// dyload

namespace dyload {

support::buffer dyload_shared_library(sl::io::span<const char> data);

} // namespace

// misc
namespace misc {

support::buffer get_wiltoncall_config(sl::io::span<const char> data);

support::buffer stdin_readline(sl::io::span<const char> data);
    
} // namespace


// internal api

namespace internal {

const sl::json::value& static_wiltoncall_config(const std::string& cf_json = "");

sl::support::observer_ptr<sl::unzip::file_index> static_modules_idx(sl::unzip::file_index* = nullptr);

} // namespace

} // namespace

#endif /* WILTON_CALL_WILTONCALL_INTERNAL_HPP */

