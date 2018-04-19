/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* Copyright (c) 2007 INRIA, Gustavo Carneiro
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation;
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* Authors: Gustavo Carneiro <gjcarneiro@gmail.com>,
*          Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
*/
#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>
#include <string>
#include <vector>
#include "ptr.h"
#include "attribute.h"
#include "object-base.h"
#include "attribute-construction-list.h"
#include "simple-ref-count.h"

namespace ns3 {

class Object;
class AttributeAccessor;
class AttributeValue;
class TraceSourceAccessor;

struct ObjectDeleter
{
 inline static void Delete (Object *object);
};

class Object : public SimpleRefCount<Object, ObjectBase, ObjectDeleter>
{
public:
 static TypeId GetTypeId (void);

 class AggregateIterator
 {
public:
   AggregateIterator ();

   bool HasNext (void) const;

   Ptr<const Object> Next (void);
private:
   friend class Object;
   AggregateIterator (Ptr<const Object> object);
   Ptr<const Object> m_object;                    
   uint32_t m_current;                            
 };

 Object ();
 virtual ~Object ();

 virtual TypeId GetInstanceTypeId (void) const;

 template <typename T>
 inline Ptr<T> GetObject (void) const;
 template <typename T>
 Ptr<T> GetObject (TypeId tid) const;
 void Dispose (void);
 void AggregateObject (Ptr<Object> other);

 AggregateIterator GetAggregateIterator (void) const;

 void Initialize (void);

 bool IsInitialized (void) const;

protected:
 virtual void NotifyNewAggregate (void);
 virtual void DoInitialize (void);
 virtual void DoDispose (void);
 Object (const Object &o);
 
private:

 template <typename T>
 friend Ptr<T> CopyObject (Ptr<T> object);
 template <typename T>
 friend Ptr<T> CopyObject (Ptr<const T> object);
 template <typename T>
 friend Ptr<T> CompleteConstruct (T *object);

 friend class ObjectFactory;
 friend class AggregateIterator;
 friend struct ObjectDeleter;

 struct Aggregates {
   uint32_t n;
   Object *buffer[1];
 };

 Ptr<Object> DoGetObject (TypeId tid) const;
 bool Check (void) const;
 bool CheckLoose (void) const;
 void SetTypeId (TypeId tid);
 void Construct (const AttributeConstructionList &attributes);

 void UpdateSortedArray (struct Aggregates *aggregates, uint32_t i) const;
 void DoDelete (void);

 TypeId m_tid;
 bool m_disposed;
 bool m_initialized;
 struct Aggregates * m_aggregates;
 uint32_t m_getObjectCount;
};

template <typename T>
Ptr<T> CopyObject (Ptr<const T> object);
template <typename T>
Ptr<T> CopyObject (Ptr<T> object);

} // namespace ns3

namespace ns3 {


/*************************************************************************
*   The Object implementation which depends on templates
*************************************************************************/

void 
ObjectDeleter::Delete (Object *object)
{
 object->DoDelete ();
}

template <typename T>
Ptr<T> 
Object::GetObject () const
{
 // This is an optimization: if the cast works (which is likely),
 // things will be pretty fast.
 T *result = dynamic_cast<T *> (m_aggregates->buffer[0]);
 if (result != 0)
   {
     return Ptr<T> (result);
   }
 // if the cast does not work, we try to do a full type check.
 Ptr<Object> found = DoGetObject (T::GetTypeId ());
 if (found != 0)
   {
     return Ptr<T> (static_cast<T *> (PeekPointer (found)));
   }
 return 0;
}

template <typename T>
Ptr<T> 
Object::GetObject (TypeId tid) const
{
 Ptr<Object> found = DoGetObject (tid);
 if (found != 0)
   {
     return Ptr<T> (static_cast<T *> (PeekPointer (found)));
   }
 return 0;
}

/*************************************************************************
*   The helper functions which need templates.
*************************************************************************/

template <typename T>
Ptr<T> CopyObject (Ptr<T> object)
{
 Ptr<T> p = Ptr<T> (new T (*PeekPointer (object)), false);
 NS_ASSERT (p->GetInstanceTypeId () == object->GetInstanceTypeId ());
 return p;
}

template <typename T>
Ptr<T> CopyObject (Ptr<const T> object)
{
 Ptr<T> p = Ptr<T> (new T (*PeekPointer (object)), false);
 NS_ASSERT (p->GetInstanceTypeId () == object->GetInstanceTypeId ());
 return p;
}

template <typename T>
Ptr<T> CompleteConstruct (T *object)
{
 object->SetTypeId (T::GetTypeId ());
 object->Object::Construct (AttributeConstructionList ());
 return Ptr<T> (object, false);
}

template <typename T>
Ptr<T> CreateObject (void)
{
 return CompleteConstruct (new T ());
}
template <typename T, typename T1>
Ptr<T> CreateObject (T1 a1)
{
 return CompleteConstruct (new T (a1));
}

template <typename T, typename T1, typename T2>
Ptr<T> CreateObject (T1 a1, T2 a2)
{
 return CompleteConstruct (new T (a1,a2));
}

template <typename T, typename T1, typename T2, typename T3>
Ptr<T> CreateObject (T1 a1, T2 a2, T3 a3)
{
 return CompleteConstruct (new T (a1,a2,a3));
}

template <typename T, typename T1, typename T2, typename T3, typename T4>
Ptr<T> CreateObject (T1 a1, T2 a2, T3 a3, T4 a4)
{
 return CompleteConstruct (new T (a1,a2,a3,a4));
}

template <typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
Ptr<T> CreateObject (T1 a1, T2 a2, T3 a3, T4 a4, T5 a5)
{
 return CompleteConstruct (new T (a1,a2,a3,a4,a5));
}

template <typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
Ptr<T> CreateObject (T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6)
{
 return CompleteConstruct (new T (a1,a2,a3,a4,a5,a6));
}

template <typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
Ptr<T> CreateObject (T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7)
{
 return CompleteConstruct (new T (a1,a2,a3,a4,a5,a6,a7));
}
} // namespace ns3

#endif /* OBJECT_H */

