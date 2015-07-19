#include <walle/net/Acceptor.h>

#include <walle/sys/wallesys.h>
#include <walle/net/Eventloop.h>
#include <walle/net/Addrinet.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <fcntl.h>
//#include <sys/types.h>
//#include <sys/stat.h>

using namespace walle::sys;
using namespace walle::net;

Acceptor::Acceptor(EventLoop* loop,  AddrInet& listenAddr, bool reuseport)
  : _loop(loop),
  _acceptSocket(true),
    _listenning(false),
    _idleFd(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
  assert(_idleFd >= 0);
  _acceptSocket.bind(listenAddr);
  _acceptSocket.setReuseAddress(reuseport);
  _acceptChannel.setUp(_loop,_acceptSocket.getFd());
  _acceptChannel.setReadCallback(
  boost::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
  _acceptChannel.disableAll();
  _acceptChannel.remove();
  ::close(_idleFd);
}

void Acceptor::listen()
{
  _loop->assertInLoopThread();

  bool rc = _acceptSocket.listen(5);
  if(rc == false ) {
	LOG_ERROR<<"bind and listen addr error";
	assert(false);
  }
  _listenning = true;
  _acceptChannel.enableReading();
}

void Acceptor::handleRead()
{
  _loop->assertInLoopThread();
  AddrInet peerAddr;

  int connfd = _acceptSocket.accept(peerAddr);
  if (connfd >= 0)
  {
    string hostport = peerAddr.toString();
    LOG_TRACE << "Accepts of " << hostport;
    if (_newConnectionCallback)
    {
      _newConnectionCallback(connfd, peerAddr);
    }
    else
    {
      ::close(connfd);
    }
  }
  else
  {
    LOG_ERROR<<"in Acceptor::handleRead";
    // Read the section named "The special problem of
    // accept()ing when you can't" in libev's doc.
    // By Marc Lehmann, author of livev.
    if (errno == EMFILE)
    {
      ::close(_idleFd);
      _idleFd = ::accept(_acceptSocket.getFd(), NULL, NULL);
      ::close(_idleFd);
      _idleFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}

