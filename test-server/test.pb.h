// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: test.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_test_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_test_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3021000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3021012 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_test_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_test_2eproto {
  static const uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_test_2eproto;
namespace ABCCD {
class AddressBook;
struct AddressBookDefaultTypeInternal;
extern AddressBookDefaultTypeInternal _AddressBook_default_instance_;
}  // namespace ABCCD
PROTOBUF_NAMESPACE_OPEN
template<> ::ABCCD::AddressBook* Arena::CreateMaybeMessage<::ABCCD::AddressBook>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace ABCCD {

// ===================================================================

class AddressBook final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:ABCCD.AddressBook) */ {
 public:
  inline AddressBook() : AddressBook(nullptr) {}
  ~AddressBook() override;
  explicit PROTOBUF_CONSTEXPR AddressBook(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  AddressBook(const AddressBook& from);
  AddressBook(AddressBook&& from) noexcept
    : AddressBook() {
    *this = ::std::move(from);
  }

  inline AddressBook& operator=(const AddressBook& from) {
    CopyFrom(from);
    return *this;
  }
  inline AddressBook& operator=(AddressBook&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const AddressBook& default_instance() {
    return *internal_default_instance();
  }
  static inline const AddressBook* internal_default_instance() {
    return reinterpret_cast<const AddressBook*>(
               &_AddressBook_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(AddressBook& a, AddressBook& b) {
    a.Swap(&b);
  }
  inline void Swap(AddressBook* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(AddressBook* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  AddressBook* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<AddressBook>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const AddressBook& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const AddressBook& from) {
    AddressBook::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(AddressBook* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "ABCCD.AddressBook";
  }
  protected:
  explicit AddressBook(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kNumsFieldNumber = 2,
    kFlagFieldNumber = 1,
  };
  // repeated int32 nums = 2;
  int nums_size() const;
  private:
  int _internal_nums_size() const;
  public:
  void clear_nums();
  private:
  int32_t _internal_nums(int index) const;
  const ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >&
      _internal_nums() const;
  void _internal_add_nums(int32_t value);
  ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >*
      _internal_mutable_nums();
  public:
  int32_t nums(int index) const;
  void set_nums(int index, int32_t value);
  void add_nums(int32_t value);
  const ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >&
      nums() const;
  ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >*
      mutable_nums();

  // int32 flag = 1;
  void clear_flag();
  int32_t flag() const;
  void set_flag(int32_t value);
  private:
  int32_t _internal_flag() const;
  void _internal_set_flag(int32_t value);
  public:

  // @@protoc_insertion_point(class_scope:ABCCD.AddressBook)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t > nums_;
    mutable std::atomic<int> _nums_cached_byte_size_;
    int32_t flag_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_test_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// AddressBook

// int32 flag = 1;
inline void AddressBook::clear_flag() {
  _impl_.flag_ = 0;
}
inline int32_t AddressBook::_internal_flag() const {
  return _impl_.flag_;
}
inline int32_t AddressBook::flag() const {
  // @@protoc_insertion_point(field_get:ABCCD.AddressBook.flag)
  return _internal_flag();
}
inline void AddressBook::_internal_set_flag(int32_t value) {
  
  _impl_.flag_ = value;
}
inline void AddressBook::set_flag(int32_t value) {
  _internal_set_flag(value);
  // @@protoc_insertion_point(field_set:ABCCD.AddressBook.flag)
}

// repeated int32 nums = 2;
inline int AddressBook::_internal_nums_size() const {
  return _impl_.nums_.size();
}
inline int AddressBook::nums_size() const {
  return _internal_nums_size();
}
inline void AddressBook::clear_nums() {
  _impl_.nums_.Clear();
}
inline int32_t AddressBook::_internal_nums(int index) const {
  return _impl_.nums_.Get(index);
}
inline int32_t AddressBook::nums(int index) const {
  // @@protoc_insertion_point(field_get:ABCCD.AddressBook.nums)
  return _internal_nums(index);
}
inline void AddressBook::set_nums(int index, int32_t value) {
  _impl_.nums_.Set(index, value);
  // @@protoc_insertion_point(field_set:ABCCD.AddressBook.nums)
}
inline void AddressBook::_internal_add_nums(int32_t value) {
  _impl_.nums_.Add(value);
}
inline void AddressBook::add_nums(int32_t value) {
  _internal_add_nums(value);
  // @@protoc_insertion_point(field_add:ABCCD.AddressBook.nums)
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >&
AddressBook::_internal_nums() const {
  return _impl_.nums_;
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >&
AddressBook::nums() const {
  // @@protoc_insertion_point(field_list:ABCCD.AddressBook.nums)
  return _internal_nums();
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >*
AddressBook::_internal_mutable_nums() {
  return &_impl_.nums_;
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >*
AddressBook::mutable_nums() {
  // @@protoc_insertion_point(field_mutable_list:ABCCD.AddressBook.nums)
  return _internal_mutable_nums();
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace ABCCD

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_test_2eproto
