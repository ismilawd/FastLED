#pragma once

// FastLED smart pointer.
//
//   * Make your subclasses inherit from fl::Referent.
//     * `class Foo: public fl::Referent {};`
//   * Use a macro to declare your smart pointer.
//     * For regular, non-template classes:
//       * `FASTLED_SMART_PTR(Foo)` -> `FooPtr` is now available.
//     * For templates use `FASTLED_SMART_PTR_NO_FWD(Foo)`
//       * `template <typename T> class Foo {}; using FooInt = Foo<int>;`
//       * `FASTLED_SAMRT_PTR_NO_FWD(FooInt)`
//       * `FooIntPtr` is now available.
//   * Instantiate from heap
//     * `FooPtr foo = NewPtr<Foo>(a, b, ...args);`
//     * Use `foo->method()` to call methods.
//   * Instantiate from stack object and disable tracking
//     * `Foo foo; FooPtr fooPtr = FooPtr::NoTracking(foo);`
///

#include "fl/namespace.h"
#include "fl/scoped_ptr.h"
#include "fl/type_traits.h"
#include "fl/referent.h"
#include "fl/bit_cast.h"
#include "fl/int.h"

// Declares a smart pointer. FASTLED_SMART_PTR(Foo) will declare a class FooPtr
// which will be a typedef of Ptr<Foo>. After this FooPtr::New(...args) can be
// used to create a new instance of Ptr<Foo>.
#define FASTLED_SMART_PTR(type)                                                \
    class type;                                                                \
    using type##Ptr = fl::Ptr<type>;

#define FASTLED_SMART_PTR_STRUCT(type)                                         \
    class type;                                                                \
    using type##Ptr = fl::Ptr<type>;

#define FASTLED_SMART_PTR_NO_FWD(type) using type##Ptr = fl::Ptr<type>;

// If you have an interface class that you want to create a smart pointer for,
// then you need to use this to bind it to a constructor.
#define FASTLED_SMART_PTR_CONSTRUCTOR(type, constructor)                       \
    template <> class PtrTraits<type> {                                        \
      public:                                                                  \
        template <typename... Args> static Ptr<type> New(Args... args) {       \
            fl::Ptr<type> ptr = constructor(args...);                          \
            return ptr;                                                        \
        }                                                                      \
    };

namespace fl {

class Referent; // Inherit this if you want your object to be able to go into a
                // Ptr, or WeakPtr.
template <typename T> class Ptr; // Reference counted smart pointer base class.
template <typename T> class WeakPtr; // Weak reference smart pointer base class.

template <typename T, typename... Args> Ptr<T> NewPtr(Args... args);

template <typename T, typename... Args> Ptr<T> NewPtrNoTracking(Args... args);

template <typename T> class PtrTraits {
  public:
    using element_type = T;
    using ptr_type = Ptr<T>;

    template <typename... Args> static Ptr<T> New(Args... args);
    static Ptr<T> New();
};

// Ptr is a reference-counted smart pointer that manages the lifetime of an
// object.
//
// It will work with any class implementing ref(), unref() and destroy().
//
// Please note that this Ptr class is "sticky" to it's referent, that is, no
// automatic conversion from raw pointers to Ptr or vice versa is allowed and
// must be done explicitly, see the Ptr::TakeOwnership() and Ptr::NoTracking()
// methods.
//
// To create a Ptr to a concrete object, it's best to use FASTLED_SMART_PTR(Foo)
// and then use FooPtr::New(...) to create a new instance of Ptr<Foo>.
//
// To create a Ptr of an interface bound to a subclass (common for driver code
// or when hiding implementation) use the Ptr<InterfaceClass>::TakeOwnership(new
// Subclass()) method.
//
// For objects created statically, use Ptr<Referent>::NoTracking(referent) to
// create a Ptr, as this will disable reference tracking but still allow it to
// be used as a Ptr.
//
// Example:
//   FASTLED_SMART_PTR(Foo);
//   class Foo: public fl::Referent {};
//   FooPtr foo = FooPtr::New();
//
// Example 2: (Manual binding to constructor)
//   class FooSubclass: public Foo {};
//   Ptr<Foo> bar = Ptr<FooSubclass>::TakeOwnership(new FooSubclass());
//
// Example 3: (Provide your own constructor so that FooPtr::New() works to
// create a FooSubclass)
//   class FooSubclass: public Foo {  // Foo is an interface, FooSubclass is an
//   implementation.
//     public:
//       static FooPtr New(int a, int b);
//   };
//   FASTLED_SMART_PTR_CONSTRUCTOR(Foo, FooSubclass::New);
//   FooPtr foo = FooPtr::New(1, 2);  // this will now work.
template <typename T> class Ptr : public PtrTraits<T> {
  public:
    friend class PtrTraits<T>;

    template <typename... Args> static Ptr<T> New(Args... args);
    
    // Used for low level allocations, typically for pointer to an
    // implementation where it needs to convert to a Ptr of a base class.
    static Ptr TakeOwnership(T *ptr) { return Ptr(ptr, true); }

    // Used for low level allocations, typically to handle memory that is
    // statically allocated where the destructor should not be called when
    // the refcount reaches 0.
    static Ptr NoTracking(T &referent) { return Ptr(&referent, false); }

    static Ptr Null() { return Ptr<T>(); }

    // Allow upcasting of Refs.
    template <typename U, typename = fl::is_derived<T, U>>
    Ptr(const Ptr<U> &refptr);

    Ptr() : referent_(nullptr) {}

    // Forbidden to convert a raw pointer to a Referent into a Ptr, because
    // it's possible that the raw pointer comes from the stack or static memory.
    Ptr(T *referent) = delete;
    Ptr &operator=(T *referent) = delete;

    Ptr(const Ptr &other);
    Ptr(Ptr &&other) noexcept;
    ~Ptr();

    Ptr &operator=(const Ptr &other);
    Ptr &operator=(Ptr &&other) noexcept;

    // Either returns the weakptr if it exists, or an empty weakptr.
    WeakPtr<T> weakRefNoCreate() const;
    WeakPtr<T> weakPtr() const { return WeakPtr<T>(*this); }

    bool operator==(const T *other) const { return referent_ == other; }
    bool operator!=(const T *other) const { return referent_ != other; }
    bool operator==(const Ptr &other) const { return referent_ == other.referent_; }
    bool operator!=(const Ptr &other) const { return referent_ != other.referent_; }
    bool operator<(const Ptr &other) const { return referent_ < other.referent_; }

    T *get() const { return referent_; }
    T *operator->() const { return referent_; }
    T &operator*() const { return *referent_; }
    explicit operator bool() const noexcept { return referent_ != nullptr; }

    void reset();
    void reset(Ptr<T> &refptr);

    // Releases the pointer from reference counting from this Ptr.
    T *release();

    void swap(Ptr &other) noexcept;
    bool isOwned() const { return referent_ && referent_->ref_count() > 0; }

  private:
    Ptr(T *referent, bool from_heap);
    T *referent_;
};

template <typename T> class WeakPtr {
  public:
    WeakPtr() : mWeakPtr(nullptr) {}

    WeakPtr(const Ptr<T> &ptr);

    template <typename U> WeakPtr(const Ptr<U> &ptr);

    WeakPtr(const WeakPtr &other);

    template <typename U>
    WeakPtr(const WeakPtr<U> &other);

    WeakPtr(WeakPtr &&other) noexcept;

    ~WeakPtr();

    operator bool() const;
    bool operator!() const;
    bool operator==(const WeakPtr &other) const;
    bool operator!=(const WeakPtr &other) const;
    bool operator==(const T *other) const;
    bool operator==(T *other) const;
    bool operator==(const Ptr<T> &other) const;
    bool operator!=(const T *other) const;

    WeakPtr &operator=(const WeakPtr &other);

    Ptr<T> lock() const;

    bool expired() const;

    void reset();

    fl::uptr ptr_value() const {
      fl::uptr val = fl::bit_cast<fl::uptr>(mWeakPtr);
      return val;
    }
    
    WeakReferent* mWeakPtr;
};

template <typename T, typename... Args> Ptr<T> NewPtr(Args... args);

template <typename T> Ptr<T> NewPtrNoTracking(T &obj);

} // namespace fl

// Template implementations must always be available for instantiation
// This achieves the goal of separating declarations from implementations
// while ensuring templates work correctly in all compilation modes
#include "fl/ptr_impl.h"
