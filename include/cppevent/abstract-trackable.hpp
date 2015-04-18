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

#ifdef DEBUG
#include <cassert>
#endif

namespace CppEvent {

// forward declaration
class AbstractTrackable;
struct Token;
struct Binding;

/**
 * @brief The abstract event node
 */
struct Binding
{
  inline Binding ()
      : trackable_object(0),
        previous(0),
        next(0),
        token(0)
  {
  }

  ~Binding ();

  AbstractTrackable* trackable_object;
  Binding* previous;
  Binding* next;
  Token* token;
};

/**
 * @brief The event source
 */
struct Token
{
  inline Token ()
      : trackable_object(0),
        previous(0),
        next(0),
        binding(0)
  {
  }

  virtual ~Token ();

  AbstractTrackable* trackable_object;
  Token* previous;
  Token* next;
  Binding* binding;

};

/**
 * @brief Abstract class for event
 */
class AbstractTrackable
{
  friend struct Binding;
  friend struct Token;

 public:

  inline AbstractTrackable ()
      : first_binding_(0), last_binding_(0)
  {
  }

  virtual ~AbstractTrackable ();

 protected:

  virtual void AuditDestroyingToken (Token* token) = 0;

  void PushBackBinding (Binding* node);

  void PushFrontBinding (Binding* node);

  void InsertBinding (int index, Binding* node);

  void RemoveAllBindings ();

  static inline void link (Token* token, Binding* binding);
  
  static inline void add_binding (AbstractTrackable* trackable,
                                  Binding* conn)
  {
    trackable->PushBackBinding(conn);
  }

  static inline void insert_binding (AbstractTrackable* trackable,
                                     Binding* conn,
                                     int index = 0)
  {
    trackable->InsertBinding(index, conn);
  }

 private:

  Binding* first_binding_;
  Binding* last_binding_;
};

// Implementations:

Binding::~Binding()
{
  if (trackable_object) {

    if (previous)
      previous->next = next;
    else
      trackable_object->first_binding_ = next;

    if (next)
      next->previous = previous;
    else
      trackable_object->last_binding_ = previous;

  } else {

    if (previous) previous->next = next;
    if (next) next->previous = previous;

  }

  previous = 0;
  next = 0;

  if (token) {
#ifdef DEBUG
    assert(token->binding == this);
#endif
    token->binding = 0;
    delete token;
    token = 0;
  }
}

Token::~Token()
{
  if (trackable_object) trackable_object->AuditDestroyingToken(this);

  if (previous) previous->next = next;
  if (next) next->previous = previous;

  previous = 0;
  next = 0;

  if (binding) {
#ifdef DEBUG
    assert(binding->token == this);
#endif
    binding->token = 0;
    delete binding;
    binding = 0;
  }
}

AbstractTrackable::~AbstractTrackable ()
{
  RemoveAllBindings();
}

void AbstractTrackable::PushBackBinding (Binding* node)
{
#ifdef DEBUG
  assert(node->trackable_object == 0);
#endif

  if (last_binding_) {
    last_binding_->next = node;
    node->previous = last_binding_;
  } else {
#ifdef DEBUG
    assert(first_binding_ == 0);
#endif
    node->previous = 0;
    first_binding_ = node;
  }
  last_binding_ = node;
  node->next = 0;
  node->trackable_object = this;
}

void AbstractTrackable::PushFrontBinding (Binding* node)
{
#ifdef DEBUG
  assert(node->trackable_object == 0);
#endif

  if (first_binding_) {
    first_binding_->previous = node;
    node->next = first_binding_;
  } else {
#ifdef DEBUG
    assert(last_binding_ == 0);
#endif
    node->next = 0;
    last_binding_ = node;
  }
  first_binding_ = node;

  node->previous = 0;
  node->trackable_object = this;
}

void AbstractTrackable::InsertBinding (int index, Binding* node)
{
#ifdef DEBUG
  assert(node->trackable_object == 0);
#endif

  if (first_binding_ == 0) {
#ifdef DEBUG
    assert(last_binding_ == 0);
#endif

    node->next = 0;
    last_binding_ = node;
    first_binding_ = node;
    node->previous = 0;
  } else {
    if (index > 0) {

      Binding* p = first_binding_;

      while (p && (index > 0)) {
        if (p->next == 0) break;
        p = p->next;
        index--;
      }

      if (index == 0) {  // insert
        node->previous = p->previous;
        node->next = p;
        p->previous->next = node;
        p->previous = node;
      } else {  // same as push back
#ifdef DEBUG
        assert(p == last_binding_);
#endif
        last_binding_->next = node;
        node->previous = last_binding_;
        last_binding_ = node;
        node->next = 0;
      }

    } else {  // same as push front
      first_binding_->previous = node;
      node->next = first_binding_;
      first_binding_ = node;
      node->previous = 0;
    }
  }
  node->trackable_object = this;
}

void AbstractTrackable::RemoveAllBindings ()
{
  Binding* tmp = 0;
  Binding* p = first_binding_;

  while (p) {
    tmp = p->next;
    delete p;
    p = tmp;
  }
}

inline void AbstractTrackable::link (Token* source, Binding* consumer)
{
#ifdef DEBUG
  assert((source->binding == 0) && (consumer->token == 0));
#endif

  source->binding = consumer;
  consumer->token = source;
}

}  // namespace CppEvent
