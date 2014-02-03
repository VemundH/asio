//
// detail/win_iocp_socket_connect_op.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2014 Vemund Handeland (vehandel at online dot no)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_WIN_IOCP_SOCKET_CONNECT_OP_HPP
#define ASIO_DETAIL_WIN_IOCP_SOCKET_CONNECT_OP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_IOCP) && (defined(_WIN32_WINNT) && _WIN32_WINNT >= 0x0501)

#include "asio/detail/addressof.hpp"
#include "asio/detail/bind_handler.hpp"
#include "asio/detail/buffer_sequence_adapter.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/handler_invoke_helpers.hpp"
#include "asio/detail/operation.hpp"
#include "asio/detail/socket_ops.hpp"
#include "asio/error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

template <typename Handler>
class win_iocp_socket_connect_op : public operation
{
public:
  ASIO_DEFINE_HANDLER_PTR(win_iocp_socket_connect_op);

  win_iocp_socket_connect_op(socket_type socket, Handler& handler)
    : operation(&win_iocp_socket_connect_op::do_complete),
      socket_(socket),
      handler_(ASIO_MOVE_CAST(Handler)(handler))
  {
  }


  static void do_complete(io_service_impl* owner, operation* base,
      const asio::error_code& result_ec,
      std::size_t /*bytes_transferred*/)
  {
    asio::error_code ec(result_ec);

    // Take ownership of the operation object.
    win_iocp_socket_connect_op* o(static_cast<win_iocp_socket_connect_op*>(base));
    ptr p = { asio::detail::addressof(o->handler_), o, o };

    if (owner)
    {
      if ( !ec )
      {
        // Need to set the SO_UPDATE_CONNECT_CONTEXT option so that shutdown, getsockname
        // and getpeername will work on the connected socket.
        socket_ops::state_type state = 0;
        socket_ops::setsockopt(o->socket_, state,
          SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT,
          0, 0, ec);
      }
    }

    ASIO_HANDLER_COMPLETION((o));

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    detail::binder1<Handler, asio::error_code>
      handler(o->handler_, ec);
    p.h = asio::detail::addressof(handler.handler_);
    p.reset();

    // Make the upcall if required.
    if (owner)
    {
      fenced_block b(fenced_block::half);
      ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_));
      asio_handler_invoke_helpers::invoke(handler, handler.handler_);
      ASIO_HANDLER_INVOCATION_END;
    }
  }

private:
  socket_type socket_;
  Handler handler_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_IOCP) && (defined(_WIN32_WINNT) && _WIN32_WINNT >= 0x0501)

#endif // ASIO_DETAIL_WIN_IOCP_SOCKET_CONNECT_OP_HPP
