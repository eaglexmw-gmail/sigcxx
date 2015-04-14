/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Freeman Zhang <zhanggyb@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <cppevent/practicalbe-invoker.hpp>

namespace CppEvent {

// forward declaration
template<typename ... ParamTypes> class Event;

template<typename ... ParamTypes>
class EventInvoker : public PracticalbeInvoker < ParamTypes... >
{
public:

  EventInvoker () = delete;

  inline EventInvoker(Event<ParamTypes...>* event);

  virtual ~EventInvoker();

  virtual void Invoke(ParamTypes... Args) override;

  inline const Event<ParamTypes...>* event () const;

private:

  Event<ParamTypes...>* event_;
};

template<typename ... ParamTypes>
inline EventInvoker<ParamTypes...>::EventInvoker (Event<
    ParamTypes...>* event)
    : PracticalbeInvoker<ParamTypes...>(), event_(event)
{
}

template<typename ... ParamTypes>
EventInvoker<ParamTypes...>::~EventInvoker()
{
}

template<typename ... ParamTypes>
void EventInvoker<ParamTypes...>::Invoke(ParamTypes... Args)
{
  event_->Invoke(Args...);
}

template<typename ... ParamTypes>
inline const Event<ParamTypes...>* EventInvoker<ParamTypes...>::event() const
{
  return event_;
}

}
