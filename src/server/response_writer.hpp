/* 
 * File:   response_writer.hpp
 * Author: alex
 *
 * Created on June 19, 2016, 9:43 PM
 */

#ifndef WILTON_SERVER_RESPONSE_WRITER_HPP
#define	WILTON_SERVER_RESPONSE_WRITER_HPP

#include <cstdint>

#include "staticlib/pimpl.hpp"

#include "common/wilton_internal_exception.hpp"

namespace wilton {
namespace server {

class response_writer : public staticlib::pimpl::pimpl_object {
protected:
    /**
     * implementation class
     */
    class impl;
public:
    /**
     * PIMPL-specific constructor
     * 
     * @param pimpl impl object
     */
    PIMPL_CONSTRUCTOR(response_writer)

    void send(const char* data, uint32_t data_len);
  
    // private api
    response_writer(void* /* staticlib::httpserver::http_response_writer_ptr&& */ writer);
};

} // namespace
}

#endif	/* WILTON_SERVER_RESPONSE_WRITER_HPP */
