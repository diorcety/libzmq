/*
    Copyright (c) 2007-2016 Contributors as noted in the AUTHORS file

    This file is part of libzmq, the ZeroMQ core engine in C++.

    libzmq is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, the Contributors give you permission to link
    this library with independent modules to produce an executable,
    regardless of the license terms of these independent modules, and to
    copy and distribute the resulting executable under terms of your choice,
    provided that you also meet, for each linked independent module, the
    terms and conditions of the license of that module. An independent
    module is a module which is not derived from or based on this library.
    If you modify this library, you must extend this exception to your
    version of the library.

    libzmq is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ZMQ_SOCKET_POLLER_HPP_INCLUDED__
#define __ZMQ_SOCKET_POLLER_HPP_INCLUDED__

#include "poller.hpp"

#if defined ZMQ_POLL_BASED_ON_POLL && !defined ZMQ_HAVE_WINDOWS
#include <poll.h>
#endif

#if defined ZMQ_HAVE_WINDOWS
#include "windows.hpp"
#elif defined ZMQ_HAVE_VXWORKS
#include <unistd.h>
#include <sys/time.h>
#include <strings.h>
#else
#include <unistd.h>
#endif

#include <vector>
#include <algorithm>

#include "socket_base.hpp"
#include "signaler.hpp"
#include "i_poll_events.hpp"

namespace zmq
{
#if defined ZMQ_HAVE_POLLER
class socket_poller_t: public i_poll_events
#else
class socket_poller_t
#endif
{
  public:
    socket_poller_t ();
    ~socket_poller_t ();

    typedef struct event_t
    {
        socket_base_t *socket;
        fd_t fd;
        void *user_data;
        short events;
    } event_t;

    int add (socket_base_t *socket, void *user_data, short events);
    int modify (socket_base_t *socket, short events);
    int remove (socket_base_t *socket);

    int add_fd (fd_t fd, void *user_data, short events);
    int modify_fd (fd_t fd, short events);
    int remove_fd (fd_t fd);

    int wait (event_t *event, int n_events, long timeout);

    inline int size (void) { return static_cast<int> (items.size ()); };

    //  Return false if object is not a socket.
    bool check_tag ();

#if defined ZMQ_HAVE_POLLER
protected:
    //  i_poll_events interface implementation.
    virtual void in_event (i_poll_events::handle_t handle_);
    virtual void out_event (i_poll_events::handle_t handle_);
    virtual void err_event (i_poll_events::handle_t handle_);
    virtual void pri_event (i_poll_events::handle_t handle_);
    virtual void timer_event (int id_);
#endif

  private:
    void zero_trail_events (zmq::socket_poller_t::event_t *events_,
                            int n_events_,
                            int found);
#if defined ZMQ_HAVE_POLLER
    int check_events (zmq::socket_poller_t::event_t *events_, int n_events_);
#else
#if defined ZMQ_POLL_BASED_ON_POLL
    int check_events (zmq::socket_poller_t::event_t *events_, int n_events_);
#elif defined ZMQ_POLL_BASED_ON_SELECT
    int check_events (zmq::socket_poller_t::event_t *events_,
                      int n_events_,
                      fd_set &inset,
                      fd_set &outset,
                      fd_set &errset);
#endif
#endif
    int adjust_timeout (zmq::clock_t &clock,
                        long timeout_,
                        uint64_t &now,
                        uint64_t &end,
                        bool &first_pass);
    void rebuild ();

    //  Used to check whether the object is a socket_poller.
    uint32_t tag;

    //  Signaler used for thread safe sockets polling
    signaler_t *signaler;
#if defined ZMQ_HAVE_POLLER
    short signaler_revents;
    base_poller_t::handle_t signaler_handle;
#endif

    typedef struct item_t
    {
        socket_base_t *socket;
        fd_t fd;
        void *user_data;
        short events;
#if defined ZMQ_HAVE_POLLER
        short revents;
        base_poller_t::handle_t handle;
#else
#if defined ZMQ_POLL_BASED_ON_POLL
        int pollfd_index;
#endif
#endif
    } item_t;

    //  List of sockets
    typedef std::vector<item_t> items_t;
    items_t items;

    //  Does the pollset needs rebuilding?
    bool need_rebuild;

    //  Should the signaler be used for the thread safe polling?
    bool use_signaler;

    //  Size of the pollset
    int poll_size;

#if defined ZMQ_HAVE_POLLER
    base_poller_t poller;
#else
#if defined ZMQ_POLL_BASED_ON_POLL
    pollfd *pollfds;
#elif defined ZMQ_POLL_BASED_ON_SELECT
    fd_set pollset_in;
    fd_set pollset_out;
    fd_set pollset_err;
    zmq::fd_t maxfd;
#endif
#endif

    socket_poller_t (const socket_poller_t &);
    const socket_poller_t &operator= (const socket_poller_t &);
};
}

#endif
