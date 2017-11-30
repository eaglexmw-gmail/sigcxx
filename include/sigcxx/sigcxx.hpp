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

#ifndef SIGCXX_SIGCXX_HPP_
#define SIGCXX_SIGCXX_HPP_

#include "sigcxx/delegate.hpp"
#include "sigcxx/deque.hpp"

#include <cstddef>
#ifdef __DEBUG__
#include <cassert>
#endif  // __DEBUG__

#ifndef __SLOT__
/**
 * @ingroup base
 * @brief A helper macro to define a slot parameter with default nullptr
 */
#define __SLOT__ ::sigcxx::SLOT slot = nullptr
#endif  // __SLOT__

namespace sigcxx {

// forward declaration
class Trackable;
class Slot;

template<typename ... ParamTypes>
class SignalT;

typedef Slot *SLOT;

/// @cond IGNORE
namespace internal {

struct Token;

template<typename ... ParamTypes>
class SignalTokenT;

/**
 * @brief A simple structure works as a list node in Trackable object
 */
struct Binding {

  Binding() = default;

  ~Binding();

  Trackable *trackable = nullptr;
  Binding *previous = nullptr;
  Binding *next = nullptr;
  Token *token = nullptr;

};

/**
 * @brief A simple structure works as a list node in Signal object
 */
struct Token {

  Token() = default;

  virtual ~Token();

  Trackable *trackable = nullptr;
  Token *previous = nullptr;
  Token *next = nullptr;
  Binding *binding = nullptr;

  BiNode slot_mark_head;

};

template<typename ... ParamTypes>
class CallableTokenT : public Token {

 public:

  CallableTokenT(const CallableTokenT &) = delete;
  CallableTokenT &operator=(const CallableTokenT &) = delete;
  CallableTokenT(CallableTokenT &&) = delete;
  CallableTokenT &operator=(CallableTokenT &&) = delete;

  CallableTokenT() = default;

  ~CallableTokenT() override = default;

  virtual void Invoke(ParamTypes ... Args) {
    // Override this in sub class
  }

};

template<typename ... ParamTypes>
class DelegateTokenT : public CallableTokenT<ParamTypes...> {

 public:

  DelegateTokenT() = delete;
  DelegateTokenT(const DelegateTokenT &) = delete;
  DelegateTokenT &operator=(const DelegateTokenT &) = delete;
  DelegateTokenT(DelegateTokenT &&) = delete;
  DelegateTokenT &operator=(DelegateTokenT &&)= delete;

  explicit DelegateTokenT(const DelegateT<void(ParamTypes...)> &d)
      : CallableTokenT<ParamTypes...>(), delegate_(d) {}

  ~DelegateTokenT() final = default;

  virtual void Invoke(ParamTypes... Args) final {
    delegate_(Args...);
  }

  inline const DelegateT<void(ParamTypes...)> &delegate() const {
    return delegate_;
  }

 private:

  DelegateT<void(ParamTypes...)> delegate_;

};

template<typename ... ParamTypes>
class SignalTokenT : public CallableTokenT<ParamTypes...> {

 public:

  SignalTokenT() = delete;
  SignalTokenT(const SignalTokenT &) = delete;
  SignalTokenT &operator=(const SignalTokenT &) = delete;
  SignalTokenT(SignalTokenT &&) = delete;
  SignalTokenT &operator=(SignalTokenT &&) = delete;

  explicit SignalTokenT(SignalT<ParamTypes...> &signal)
      : CallableTokenT<ParamTypes...>(), signal_(&signal) {}

  ~SignalTokenT() final = default;

  virtual void Invoke(ParamTypes... Args) final {
    signal_->Emit(Args...);
  }

  const SignalT<ParamTypes...> *signal() const {
    return signal_;
  }

 private:

  SignalT<ParamTypes...> *signal_;

};

}// namespace details
/// @endcond

/**
 * @ingroup base
 * @brief Iterator and signature to a slot method
 *
 * A Slot object is created and destroyed when a signal is being emitting.
 *
 * It has two main purposes:
 *   - Works as an iterator
 *   - The last parameter in a slot method
 *
 * A Signal holds a list of token to support multicast, when it's being
 * emitting, it create a simple Slot object and use it as an iterater and call
 * each delegate (@ref Delegate) to the slot method or another signal.
 */
class Slot {

  friend struct internal::Token;
  friend class Trackable;

  template<typename ... ParamTypes> friend
  class SignalT;

 public:

  class Mark : public BiNode {

   public:

    Mark() = delete;

    explicit Mark(Slot *slot)
        : slot_(slot) {}

    ~Mark() final = default;

    Slot *slot() const { return slot_; }

   private:

    Slot *slot_ = nullptr;

  };

  Slot() = delete;
  Slot(const Slot &) = delete;
  Slot &operator=(const Slot &) = delete;
  Slot(Slot &&) = delete;
  Slot &operator=(Slot &&) = delete;

  /**
   * @brief Get the Signal object which is just calling this slot
   */
  template<typename ... ParamTypes>
  SignalT<ParamTypes...> *signal() const {
    return dynamic_cast<SignalT<ParamTypes...> *>(token_->trackable);
  }

  /**
   * @brief The trackable object in which the slot method is being called
   * @return The trackable object receiving signal
   */
  Trackable *binding_trackable() const {
    return token_->binding->trackable;
  }

 private:

  explicit Slot(internal::Token *token)
      : token_(token), skip_(false), mark_(this) {}

  ~Slot() = default;

  Slot &operator++() {
    token_ = token_->next;
    return *this;
  }

  Slot &operator--() {
    token_ = token_->previous;
    return *this;
  }

  internal::Token *token_;

  bool skip_;

  Mark mark_;

};

/**
 * @ingroup base
 * @brief The basic class for an object which can provide slot methods
 */
class Trackable {

  friend struct internal::Binding;
  friend struct internal::Token;

  template<typename ... ParamTypes> friend
  class SignalT;

 public:

  /**
   * @brief Default constructor
   */
  Trackable() = default;

  /**
   * @brief Copy constructor
   *
   * Do nothing in copy constructor.
   */
  Trackable(const Trackable &) {}

  /**
   * @brief Destructor
   */
  virtual ~Trackable();

  /**
   * @brief Copy assignment
   *
   * Do nothing in copy assignment.
   */
  Trackable &operator=(const Trackable &) {
    return *this;
  }

  /**
   * @brief Count connections to the given slot method
   */
  template<typename T, typename ... ParamTypes>
  int CountSignalBindings(void (T::*method)(ParamTypes...)) const;

  /**
   * @brief Count all connections
   */
  int CountSignalBindings() const;

 protected:

  /**
   * @brief Break the connection to a signal by given slot
   *
   * This is the fastest way to disconnect from a signal via the slot parameter.
   */
  void UnbindSignal(SLOT slot);

  /**
    * @brief Break the all connections to this object
    */
  void UnbindAllSignals();

  /**
    * @brief Break all connections to the given slot method of this object
    */
  template<typename T, typename ... ParamTypes>
  void UnbindAllSignalsTo(void (T::*method)(ParamTypes...));

  virtual void AuditDestroyingToken(internal::Token *token) {}

 private:

  void PushBackBinding(internal::Binding *binding);

  void PushFrontBinding(internal::Binding *binding);

  void InsertBinding(int index, internal::Binding *binding);

  static inline void link(internal::Token *token, internal::Binding *binding) {
#ifdef __DEBUG__
    assert((nullptr == token->binding) && (nullptr == binding->token));
#endif

    token->binding = binding;
    binding->token = token;
  }

  static inline void push_front(Trackable *trackable,
                                internal::Binding *binding) {
    trackable->PushBackBinding(binding);
  }

  static inline void push_back(Trackable *trackable,
                               internal::Binding *binding) {
    trackable->PushBackBinding(binding);
  }

  static inline void insert(Trackable *trackable,
                            internal::Binding *binding,
                            int index = 0) {
    trackable->InsertBinding(index, binding);
  }

  internal::Binding *first_binding_ = nullptr;
  internal::Binding *last_binding_ = nullptr;

};

template<typename T, typename ... ParamTypes>
void Trackable::UnbindAllSignalsTo(void (T::*method)(ParamTypes...)) {
  internal::Binding *it = last_binding_;
  internal::Binding *tmp = nullptr;
  internal::DelegateTokenT<ParamTypes...> *delegate_token = nullptr;

  while (it) {
    tmp = it;
    it = it->previous;

    delegate_token = dynamic_cast<internal::DelegateTokenT<ParamTypes...> * > (tmp->token);
    if (delegate_token && (delegate_token->delegate().template Equal<T>((T *) this, method))) {
      delete tmp;
    }
  }
}

template<typename T, typename ... ParamTypes>
int Trackable::CountSignalBindings(void (T::*method)(ParamTypes...)) const {
  int count = 0;
  internal::DelegateTokenT<ParamTypes...> *delegate_token = nullptr;

  for (internal::Binding *p = first_binding_; p; p = p->next) {
    delegate_token = dynamic_cast<internal::DelegateTokenT<ParamTypes...> * > (p->token);
    if (delegate_token && (delegate_token->delegate().template Equal<T>((T *) this, method))) {
      count++;
    }
  }
  return count;
}

/**
 * @ingroup base
 * @brief A template class which can emit signal(s)
 */
template<typename ... ParamTypes>
class SignalT : public Trackable {

  friend class Trackable;

 public:

  SignalT(const SignalT &) = delete;
  SignalT &operator=(const SignalT &) = delete;
  SignalT(SignalT &&) = delete;
  SignalT &operator=(SignalT &&) = delete;

  SignalT() = default;

  ~SignalT() override {
    DisconnectAll();
  }

  /**
   * @brief Connect this signal to a slot method in a observer
   */
  template<typename T>
  void Connect(T *obj, void (T::*method)(ParamTypes..., SLOT), int index = -1);

  void Connect(SignalT<ParamTypes...> &other, int index = -1);

  /**
   * @brief Disconnect all delegates to a method
   */
  template<typename T>
  void DisconnectAll(T *obj, void (T::*method)(ParamTypes..., SLOT));

  /**
   * @brief Disconnect all signals
   */
  void DisconnectAll(SignalT<ParamTypes...> &other);

  /**
   * @brief Disconnect delegats to a method by given start position and counts
   * @tparam T
   * @param obj
   * @param method
   * @param start_pos
   * @param counts
   * @return A integer number of how many delegates are found and disconnected
   *
   * By the default parameters this disconnect the last delegate to a method.
   */
  template<typename T>
  int Disconnect(T *obj, void (T::*method)(ParamTypes..., SLOT), int start_pos = -1, int counts = 1);

  /**
   * @brief Disconnect connections to a signal by given start position and counts
   * @param other
   * @param start_pos
   * @param counts
   * @return A integer number of how many signals are found and disconnected
   *
   * By the default parameters this disconnect the last signal.
   */
  int Disconnect(SignalT<ParamTypes...> &other, int start_pos = -1, int counts = 1);

  /**
   * @brief Disconnect any kind of connections from the start position
   * @param start_pos
   * @param counts How many connections to be break, if it's negative or a very big number,
   *        this is the same as DisconnectAll()
   * @return
   */
  int Disconnect(int start_pos = -1, int counts = 1);

  /**
   * @brief Disconnect all
   */
  void DisconnectAll();

  template<typename T>
  bool IsConnectedTo(T *obj, void (T::*method)(ParamTypes..., SLOT)) const;

  bool IsConnectedTo(const SignalT<ParamTypes...> &other) const;

  bool IsConnectedTo(const Trackable *obj) const;

  template<typename T>
  int CountConnections(T *obj, void (T::*method)(ParamTypes..., SLOT)) const;

  int CountConnections(const SignalT<ParamTypes...> &other) const;

  int CountConnections() const;

  void Emit(ParamTypes ... Args);

  inline void operator()(ParamTypes ... Args) {
    Emit(Args...);
  }

  void PushBackToken(internal::Token *token);

  void PushFrontToken(internal::Token *token);

  void InsertToken(int index, internal::Token *token);

  inline internal::Token *first_token() const {
    return first_token_;
  }

  inline internal::Token *last_token() const {
    return last_token_;
  }

 protected:

  void AuditDestroyingToken(internal::Token *token) final;

 private:

  internal::Token *first_token_ = nullptr;
  internal::Token *last_token_ = nullptr;

};

// Signal implementation:

template<typename ... ParamTypes>
template<typename T>
void SignalT<ParamTypes...>::Connect(T *obj, void (T::*method)(ParamTypes..., SLOT), int index) {
  internal::Binding *downstream = new internal::Binding;
  DelegateT<void(ParamTypes..., SLOT)> d =
      DelegateT<void(ParamTypes..., SLOT)>::template FromMethod<T>(obj, method);
  internal::DelegateTokenT<ParamTypes..., SLOT> *token = new internal::DelegateTokenT<
      ParamTypes..., SLOT>(d);

  link(token, downstream);
  InsertToken(index, token);
  push_back(obj, downstream);  // always push back binding, don't care about the position in observer
}

template<typename ... ParamTypes>
void SignalT<ParamTypes...>::Connect(SignalT<ParamTypes...> &other, int index) {
  internal::SignalTokenT<ParamTypes...> *token = new internal::SignalTokenT<ParamTypes...>(
      other);
  internal::Binding *binding = new internal::Binding;

  link(token, binding);
  InsertToken(index, token);
  push_back(&other, binding);  // always push back binding, don't care about the position in observer
}

template<typename ... ParamTypes>
template<typename T>
void SignalT<ParamTypes...>::DisconnectAll(T *obj, void (T::*method)(ParamTypes..., SLOT)) {
  internal::DelegateTokenT<ParamTypes..., SLOT> *delegate_token = nullptr;
  internal::Token *it = last_token_;
  internal::Token *tmp = nullptr;

  while (it) {
    tmp = it;
    it = it->previous;

    if (tmp->binding->trackable == obj) {
      delegate_token = dynamic_cast<internal::DelegateTokenT<ParamTypes..., SLOT> * > (tmp);
      if (delegate_token && (delegate_token->delegate().template Equal<T>(obj, method))) {
        delete tmp;
      }
    }
  }
}

template<typename ... ParamTypes>
void SignalT<ParamTypes...>::DisconnectAll(SignalT<ParamTypes...> &other) {
  internal::SignalTokenT<ParamTypes...> *signal_token = nullptr;
  internal::Token *it = last_token_;
  internal::Token *tmp = nullptr;

  while (it) {
    tmp = it;
    it = it->previous;

    if (tmp->binding->trackable == (&other)) {
      signal_token = dynamic_cast<internal::SignalTokenT<ParamTypes...> * > (tmp);
      if (signal_token && (signal_token->signal() == (&other))) {
        delete tmp;
      }
    }
  }
}

template<typename ... ParamTypes>
template<typename T>
int SignalT<ParamTypes...>::Disconnect(T *obj, void (T::*method)(ParamTypes..., SLOT), int start_pos, int counts) {
  internal::DelegateTokenT<ParamTypes..., SLOT> *delegate_token = nullptr;
  internal::Token *it = nullptr;
  internal::Token *tmp = nullptr;
  int ret_count = 0;

  if (start_pos >= 0) {
    it = first_token_;
    while (it && (start_pos > 0)) {
      it = it->next;
      start_pos--;
    }

    while (it) {
      tmp = it;
      it = it->next;

      if (tmp->binding->trackable == obj) {
        delegate_token = dynamic_cast<internal::DelegateTokenT<ParamTypes..., SLOT> * > (tmp);
        if (delegate_token && (delegate_token->delegate().template Equal<T>(obj, method))) {
          ret_count++;
          counts--;
          delete tmp;
        }
      }
      if (counts == 0) break;
    }
  } else {
    it = last_token_;
    while (it && (start_pos < -1)) {
      it = it->previous;
      start_pos++;
    }

    while (it) {
      tmp = it;
      it = it->previous;

      if (tmp->binding->trackable == obj) {
        delegate_token = dynamic_cast<internal::DelegateTokenT<ParamTypes..., SLOT> * > (tmp);
        if (delegate_token && (delegate_token->delegate().template Equal<T>(obj, method))) {
          ret_count++;
          counts--;
          delete tmp;
        }
      }
      if (counts == 0) break;
    }
  }

  return ret_count;
}

template<typename ... ParamTypes>
int SignalT<ParamTypes...>::Disconnect(SignalT<ParamTypes...> &other, int start_pos, int counts) {
  internal::SignalTokenT<ParamTypes...> *signal_token = nullptr;
  internal::Token *it = nullptr;
  internal::Token *tmp = nullptr;
  int ret_count = 0;

  if (start_pos >= 0) {
    it = first_token_;
    while (it && (start_pos > 0)) {
      it = it->next;
      start_pos--;
    }

    while (it) {
      tmp = it;
      it = it->next;

      if (tmp->binding->trackable == (&other)) {
        signal_token = dynamic_cast<internal::SignalTokenT<ParamTypes...> * > (tmp);
        if (signal_token && (signal_token->signal() == (&other))) {
          ret_count++;
          counts--;
          delete tmp;
        }
      }
      if (counts == 0) break;
    }

  } else {
    it = last_token_;
    while (it && (start_pos < -1)) {
      it = it->previous;
      start_pos++;
    }

    while (it) {
      tmp = it;
      it = it->previous;

      if (tmp->binding->trackable == (&other)) {
        signal_token = dynamic_cast<internal::SignalTokenT<ParamTypes...> * > (tmp);
        if (signal_token && (signal_token->signal() == (&other))) {
          ret_count++;
          counts--;
          delete tmp;
        }
      }
      if (counts == 0) break;
    }

  }

  return ret_count;
}

template<typename ... ParamTypes>
int SignalT<ParamTypes...>::Disconnect(int start_pos, int counts) {
  internal::Token *it = nullptr;
  internal::Token *tmp = nullptr;
  int ret_count = 0;

  if (start_pos >= 0) {
    it = first_token_;
    while (it && (start_pos > 0)) {
      it = it->next;
      start_pos--;
    }

    while (it) {
      tmp = it;
      it = it->next;

      ret_count++;
      counts--;
      delete tmp;

      if (counts == 0) break;
    }

  } else {
    it = last_token_;
    while (it && (start_pos < -1)) {
      it = it->previous;
      start_pos++;
    }

    while (it) {
      tmp = it;
      it = it->previous;

      ret_count++;
      counts--;
      delete tmp;

      if (counts == 0) break;
    }

  }

  return ret_count;
}

template<typename ... ParamTypes>
template<typename T>
bool SignalT<ParamTypes...>::IsConnectedTo(T *obj, void (T::*method)(ParamTypes..., SLOT)) const {
  internal::DelegateTokenT<ParamTypes..., SLOT> *delegate_token = nullptr;

  for (internal::Token *it = first_token_; it; it = it->next) {
    if (it->binding->trackable == obj) {
      delegate_token = dynamic_cast<internal::DelegateTokenT<ParamTypes..., SLOT> * > (it);
      if (delegate_token && (delegate_token->delegate().template Equal<T>(obj, method))) {
        return true;
      }
    }
  }
  return false;
}

template<typename ... ParamTypes>
bool SignalT<ParamTypes...>::IsConnectedTo(const SignalT<ParamTypes...> &other) const {
  internal::SignalTokenT<ParamTypes...> *signal_token = nullptr;

  for (internal::Token *it = first_token_; it; it = it->next) {
    if (it->binding->trackable == (&other)) {
      signal_token = dynamic_cast<internal::SignalTokenT<ParamTypes...> * > (it);
      if (signal_token && (signal_token->signal() == (&other))) {
        return true;
      }
    }
  }
  return false;
}

template<typename ... ParamTypes>
bool SignalT<ParamTypes...>::IsConnectedTo(const Trackable *obj) const {
  internal::Token *token = first_token();
  internal::Binding *binding = obj->first_binding_;

  while (token && binding) {

    if (token->binding->trackable == obj) return true;
    if (binding->token->trackable == this) return true;

    token = token->next;
    binding = binding->next;
  }

  return false;
}

template<typename ... ParamTypes>
template<typename T>
int SignalT<ParamTypes...>::CountConnections(T *obj, void (T::*method)(ParamTypes..., SLOT)) const {
  int count = 0;
  internal::DelegateTokenT<ParamTypes..., SLOT> *delegate_token = nullptr;

  for (internal::Token *it = first_token_; it; it = it->next) {
    if (it->binding->trackable == obj) {
      delegate_token = dynamic_cast<internal::DelegateTokenT<ParamTypes..., SLOT> * > (it);
      if (delegate_token && (delegate_token->delegate().template Equal<T>(obj, method))) {
        count++;
      }
    }
  }
  return count;
}

template<typename ... ParamTypes>
int SignalT<ParamTypes...>::CountConnections(const SignalT<ParamTypes...> &other) const {
  int count = 0;
  internal::SignalTokenT<ParamTypes...> *signal_token = nullptr;

  for (internal::Token *it = first_token_; it; it = it->next) {
    if (it->binding->trackable == (&other)) {
      signal_token = dynamic_cast<internal::SignalTokenT<ParamTypes...> * > (it);
      if (signal_token && (signal_token->signal() == (&other))) {
        count++;
      }
    }
  }
  return count;
}

template<typename ... ParamTypes>
int SignalT<ParamTypes...>::CountConnections() const {
  int count = 0;
  for (internal::Token *it = first_token_; it; it = it->next) {
    count++;
  }
  return count;
}

template<typename ... ParamTypes>
void SignalT<ParamTypes...>::Emit(ParamTypes ... Args) {
  Slot slot(first_token());

  while (nullptr != slot.token_) {
    slot.token_->slot_mark_head.PushBack(&slot.mark_);
    static_cast<internal::CallableTokenT<ParamTypes..., SLOT> * > (slot.token_)->Invoke(Args..., &slot);

    if (slot.skip_) {
      slot.skip_ = false;
    } else {
      ++slot;
    }
  }
}

template<typename ... ParamTypes>
void SignalT<ParamTypes...>::AuditDestroyingToken(internal::Token *token) {
  if (token == first_token_) first_token_ = token->next;
  if (token == last_token_) last_token_ = token->previous;
}

template<typename ... ParamTypes>
void SignalT<ParamTypes...>::PushBackToken(internal::Token *token) {
#ifdef __DEBUG__
  assert(nullptr == token->trackable);
#endif

  if (last_token_) {
    last_token_->next = token;
    token->previous = last_token_;
  } else {
#ifdef __DEBUG__
    assert(nullptr == first_token_);
#endif
    token->previous = nullptr;
    first_token_ = token;
  }
  last_token_ = token;
  token->next = nullptr;
  token->trackable = this;
}

template<typename ... ParamTypes>
void SignalT<ParamTypes...>::PushFrontToken(internal::Token *token) {
#ifdef __DEBUG__
  assert(nullptr == token->trackable);
#endif

  if (first_token_) {
    first_token_->previous = token;
    token->next = first_token_;
  } else {
#ifdef __DEBUG__
    assert(nullptr == last_token_);
#endif
    token->next = nullptr;
    last_token_ = token;
  }
  first_token_ = token;

  token->previous = nullptr;
  token->trackable = this;
}

template<typename ... ParamTypes>
void SignalT<ParamTypes...>::InsertToken(int index, internal::Token *token) {
#ifdef __DEBUG__
  assert(nullptr == token->trackable);
#endif

  if (nullptr == first_token_) {
#ifdef __DEBUG__
    assert(nullptr == last_token_);
#endif
    token->next = nullptr;
    last_token_ = token;
    first_token_ = token;
    token->previous = nullptr;
  } else {
    if (index >= 0) {

      internal::Token *p = first_token_;
#ifdef __DEBUG__
      assert(p != nullptr);
#endif

      while (p && (index > 0)) {
        p = p->next;
        index--;
      }

      if (p) {  // insert before p

        token->previous = p->previous;
        token->next = p;

        if (p->previous) p->previous->next = token;
        else first_token_ = token;

        p->previous = token;

      } else {  // push back

        last_token_->next = token;
        token->previous = last_token_;
        last_token_ = token;
        token->next = nullptr;

      }

    } else {

      internal::Token *p = last_token_;
#ifdef __DEBUG__
      assert(p != nullptr);
#endif

      while (p && (index < -1)) {
        p = p->previous;
        index++;
      }

      if (p) {  // insert after p

        token->next = p->next;
        token->previous = p;

        if (p->next) p->next->previous = token;
        else last_token_ = token;

        p->next = token;

      } else {  // push front

        first_token_->previous = token;
        token->next = first_token_;
        first_token_ = token;
        token->previous = nullptr;

      }

    }
  }
  token->trackable = this;
}

template<typename ... ParamTypes>
void SignalT<ParamTypes...>::DisconnectAll() {
  internal::Token *it = first_token_;
  internal::Token *tmp = nullptr;

  while (it) {
    tmp = it;
    it = it->next;
    delete tmp;
  }
}

/**
 * @ingroup base
 * @brief A reference to a corresponding signal
 */
template<typename ... ParamTypes>
class SignalRefT {

 public:

  SignalRefT() = delete;
  SignalRefT &operator=(const SignalRefT &) = delete;

  SignalRefT(SignalT<ParamTypes...> &signal)
      : signal_(&signal) {}

  SignalRefT(const SignalRefT &orig)
      : signal_(orig.signal_) {}

  ~SignalRefT() = default;

  template<typename T>
  void Connect(T *obj, void (T::*method)(ParamTypes..., SLOT), int index = -1) {
    signal_->Connect(obj, method, index);
  }

  void Connect(SignalT<ParamTypes...> &signal, int index = -1) {
    signal_->Connect(signal, index);
  }

  template<typename T>
  void DisconnectAll(T *obj, void (T::*method)(ParamTypes..., SLOT)) {
    signal_->DisconnectAll(obj, method);
  }

  void DisconnectAll(SignalT<ParamTypes...> &signal) {
    signal_->DisconnectAll(signal);
  }

  template<typename T>
  int Disconnect(T *obj, void (T::*method)(ParamTypes..., SLOT), int start_pos = -1, int counts = 1) {
    return signal_->Disconnect(obj, method, start_pos, counts);
  }

  int Disconnect(SignalT<ParamTypes...> &signal, int start_pos = -1, int counts = 1) {
    return signal_->Disconnect(signal, start_pos, counts);
  }

  int Disconnect(int start_pos = -1, int counts = 1) {
    return signal_->Disconnect(start_pos, counts);
  }

  void DisconnectAll() {
    signal_->DisconnectAll();
  }

  template<typename T>
  bool IsConnectedTo(T *obj, void (T::*method)(ParamTypes..., SLOT)) const {
    return signal_->IsConnectedTo(obj, method);
  }

  bool IsConnectedTo(const SignalT<ParamTypes...> &signal) const {
    return signal_->IsConnectedTo(signal);
  }

  bool IsConnectedTo(const Trackable *obj) const {
    return signal_->IsConnectedTo(obj);
  }

  template<typename T>
  int CountConnections(T *obj, void (T::*method)(ParamTypes..., SLOT)) const {
    return signal_->CountConnections(obj, method);
  }

  int CountConnections(const SignalT<ParamTypes...> &signal) const {
    return signal_->CountConnections(signal);
  }

  int CountConnections() const {
    return signal_->CountConnections();
  }

  int CountBindings() const {
    return signal_->CountSignalBindings();
  }

 private:

  SignalT<ParamTypes...> *signal_;

};

} // namespace sigcxx

#endif
