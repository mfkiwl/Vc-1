/*  This file is part of the Vc library.

    Copyright (C) 2009 Matthias Kretz <kretz@kde.org>

    Vc is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    Vc is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Vc.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef SSE_VECTOR_H
#define SSE_VECTOR_H

#include "intrinsics.h"
#include "vectorbase.h"
#include "vectorhelper.h"
#include "mask.h"
#include <algorithm>

#ifdef isfinite
#undef isfinite
#endif
#ifdef isnan
#undef isnan
#endif

namespace SSE
{
    enum { VectorAlignment = 16 };

template<typename T>
class WriteMaskedVector
{
    friend class Vector<T>;
    typedef typename VectorBase<T>::MaskType Mask;
    public:
        //prefix
        inline Vector<T> &operator++() {
            vec->data() = VectorHelper<T>::add(vec->data(),
                    VectorHelper<T>::notMaskedToZero(VectorHelper<T>::one(), mask.data())
                    );
            return *vec;
        }
        inline Vector<T> &operator--() {
            vec->data() = VectorHelper<T>::sub(vec->data(),
                    VectorHelper<T>::notMaskedToZero(VectorHelper<T>::one(), mask.data())
                    );
            return *vec;
        }
        //postfix
        inline Vector<T> operator++(int) {
            Vector<T> ret(*vec);
            vec->data() = VectorHelper<T>::add(vec->data(),
                    VectorHelper<T>::notMaskedToZero(VectorHelper<T>::one(), mask.data())
                    );
            return ret;
        }
        inline Vector<T> operator--(int) {
            Vector<T> ret(*vec);
            vec->data() = VectorHelper<T>::sub(vec->data(),
                    VectorHelper<T>::notMaskedToZero(VectorHelper<T>::one(), mask.data())
                    );
            return ret;
        }

        inline Vector<T> &operator+=(Vector<T> x) {
            vec->data() = VectorHelper<T>::add(vec->data(), VectorHelper<T>::notMaskedToZero(x.data(), mask.data()));
            return *vec;
        }
        inline Vector<T> &operator-=(Vector<T> x) {
            vec->data() = VectorHelper<T>::sub(vec->data(), VectorHelper<T>::notMaskedToZero(x.data(), mask.data()));
            return *vec;
        }
        inline Vector<T> &operator*=(Vector<T> x) {
            vec->data() = VectorHelper<T>::mul(vec->data(), x.data(), mask.data());
            return *vec;
        }
        inline Vector<T> &operator/=(Vector<T> x) {
            vec->data() = VectorHelper<T>::div(vec->data(), x.data(), mask.data());
            return *vec;
        }

        inline Vector<T> &operator=(Vector<T> x) {
            vec->assign(x, mask);
            return *vec;
        }
    private:
        WriteMaskedVector(Vector<T> *v, Mask k) : vec(v), mask(k) {}
        Vector<T> *vec;
        Mask mask;
};

template<typename T>
class Vector : public VectorBase<T>
{
    public:
        typedef VectorBase<T> Base;
        enum { Size = Base::Size };
        typedef typename Base::VectorType VectorType;
        typedef typename Base::EntryType  EntryType;
        typedef Vector<typename IndexTypeHelper<Size>::Type> IndexType;
        typedef typename Base::MaskType Mask;

        /**
         * uninitialized
         */
        inline Vector() {}

        /**
         * initialized to 0 in all 128 bits
         */
        inline explicit Vector(VectorSpecialInitializerZero::ZEnum) : Base(VectorHelper<VectorType>::zero()) {}

        /**
         * initialized to 1 for all entries in the vector
         */
        inline explicit Vector(VectorSpecialInitializerOne::OEnum) : Base(VectorHelper<T>::one()) {}

        /**
         * initialized to 0, 1 (, 2, 3 (, 4, 5, 6, 7))
         */
        inline explicit Vector(VectorSpecialInitializerIndexesFromZero::IEnum) : Base(VectorHelper<VectorType>::load(Base::_IndexesFromZero())) {}

        /**
         * initialize with given _M128 vector
         */
        inline Vector(const VectorType &x) : Base(x) {}

        template<typename OtherT>
        explicit inline Vector(const Vector<OtherT> &x) : Base(StaticCastHelper<OtherT, T>::cast(x.data())) {}

        /**
         * initialize all values with the given value
         */
        inline Vector(EntryType a)
        {
            data() = VectorHelper<T>::set(a);
        }

        /**
         * Initialize the vector with the given data. \param x must point to 64 byte aligned 512
         * byte data.
         */
        inline explicit Vector(const EntryType *x) : Base(VectorHelper<VectorType>::load(x)) {}

        inline explicit Vector(const Vector<typename CtorTypeHelper<T>::Type> *a)
            : Base(VectorHelper<T>::concat(a[0].data(), a[1].data()))
        {}

        inline void expand(Vector<typename ExpandTypeHelper<T>::Type> *x) const
        {
            if (Size == 8u) {
                x[0].data() = VectorHelper<T>::expand0(data());
                x[1].data() = VectorHelper<T>::expand1(data());
            }
        }

        static inline Vector broadcast4(const EntryType *x) { return Vector<T>(x); }

        inline void load(const EntryType *mem) { data() = VectorHelper<VectorType>::load(mem); }

        static inline Vector loadUnaligned(const EntryType *mem) { return VectorHelper<VectorType>::loadUnaligned(mem); }

        inline void makeZero() { data() = VectorHelper<VectorType>::zero(); }

        /**
         * Set all entries to zero where the mask is set. I.e. a 4-vector with a mask of 0111 would
         * set the last three entries to 0.
         */
        inline void makeZero(const Mask &k) { data() = VectorHelper<VectorType>::andnot_(mm128_reinterpret_cast<VectorType>(k.data()), data()); }

        /**
         * Store the vector data to the given memory. The memory must be 64 byte aligned and of 512
         * bytes size.
         */
        inline void store(EntryType *mem) const { VectorHelper<VectorType>::store(mem, data()); }

        /**
         * Non-temporal store variant. Writes to the memory without polluting the cache.
         */
        inline void storeStreaming(EntryType *mem) const { VectorHelper<VectorType>::storeStreaming(mem, data()); }

        inline const Vector<T> &dcba() const { return *this; }
        inline const Vector<T> cdab() const { return reinterpret_cast<VectorType>(_mm_shuffle_epi32(data(), _MM_SHUFFLE(2, 3, 0, 1))); }
        inline const Vector<T> badc() const { return reinterpret_cast<VectorType>(_mm_shuffle_epi32(data(), _MM_SHUFFLE(1, 0, 3, 2))); }
        inline const Vector<T> aaaa() const { return reinterpret_cast<VectorType>(_mm_shuffle_epi32(data(), _MM_SHUFFLE(0, 0, 0, 0))); }
        inline const Vector<T> bbbb() const { return reinterpret_cast<VectorType>(_mm_shuffle_epi32(data(), _MM_SHUFFLE(1, 1, 1, 1))); }
        inline const Vector<T> cccc() const { return reinterpret_cast<VectorType>(_mm_shuffle_epi32(data(), _MM_SHUFFLE(2, 2, 2, 2))); }
        inline const Vector<T> dddd() const { return reinterpret_cast<VectorType>(_mm_shuffle_epi32(data(), _MM_SHUFFLE(3, 3, 3, 3))); }
        inline const Vector<T> dacb() const { return reinterpret_cast<VectorType>(_mm_shuffle_epi32(data(), _MM_SHUFFLE(3, 0, 2, 1))); }

        inline Vector(const EntryType *array, const IndexType &indexes) {
            VectorHelperSize<T>::gather(*this, indexes, array);
        }
        inline Vector(const EntryType *array, const IndexType &indexes, const Mask &mask) {
            GeneralHelpers::maskedGatherHelper(*this, indexes, mask.toInt(), array);
        }

        inline void gather(const EntryType *array, const IndexType &indexes) {
            VectorHelperSize<T>::gather(*this, indexes, array);
        }
        inline void gather(const EntryType *array, const IndexType &indexes, const Mask &mask) {
            GeneralHelpers::maskedGatherHelper(*this, indexes, mask.toInt(), array);
        }

        inline void scatter(EntryType *array, const IndexType &indexes) const {
            VectorHelperSize<T>::scatter(*this, indexes, array);
        }
        inline void scatter(EntryType *array, const IndexType &indexes, const Mask &mask) const {
            VectorHelperSize<T>::scatter(*this, indexes, mask.shiftMask(), array);
        }

        /**
         * \param array An array of objects where one member should be gathered
         * \param member1 A member pointer to the member of the class/struct that should be gathered
         * \param indexes The indexes in the array. The correct offsets are calculated
         *                automatically.
         * \param mask Optional mask to select only parts of the vector that should be gathered
         */
        template<typename S1> inline Vector(const S1 *array, const EntryType S1::* member1, const IndexType &indexes) {
            VectorHelperSize<T>::gather(*this, indexes, array, member1);
        }
        template<typename S1> inline Vector(const S1 *array, const EntryType S1::* member1,
                const IndexType &indexes, const Mask &mask) {
            GeneralHelpers::maskedGatherStructHelper(*this, indexes, mask.toInt(), &(array[0].*(member1)), sizeof(S1));
        }
        template<typename S1, typename S2> inline Vector(const S1 *array, const S2 S1::* member1,
                const EntryType S2::* member2, const IndexType &indexes) {
            VectorHelperSize<T>::gather(*this, indexes, array, member1, member2);
        }
        template<typename S1, typename S2> inline Vector(const S1 *array, const S2 S1::* member1,
                const EntryType S2::* member2, const IndexType &indexes, const Mask &mask) {
            GeneralHelpers::maskedGatherStructHelper(*this, indexes, mask.toInt(), &(array[0].*(member1).*(member2)), sizeof(S1));
        }

        template<typename S1> inline void gather(const S1 *array, const EntryType S1::* member1,
                const IndexType &indexes) {
            VectorHelperSize<T>::gather(*this, indexes, array, member1);
        }
        template<typename S1> inline void gather(const S1 *array, const EntryType S1::* member1,
                const IndexType &indexes, const Mask &mask) {
            GeneralHelpers::maskedGatherStructHelper(*this, indexes, mask.toInt(), &(array[0].*(member1)), sizeof(S1));
        }
        template<typename S1, typename S2> inline void gather(const S1 *array, const S2 S1::* member1,
                const EntryType S2::* member2, const IndexType &indexes) {
            VectorHelperSize<T>::gather(*this, indexes, array, member1, member2);
        }
        template<typename S1, typename S2> inline void gather(const S1 *array, const S2 S1::* member1,
                const EntryType S2::* member2, const IndexType &indexes, const Mask &mask) {
            GeneralHelpers::maskedGatherStructHelper(*this, indexes, mask.toInt(), &(array[0].*(member1).*(member2)), sizeof(S1));
        }

        template<typename S1> inline void scatter(S1 *array, EntryType S1::* member1,
                const IndexType &indexes) const {
            VectorHelperSize<T>::scatter(*this, indexes, array, member1);
        }
        template<typename S1> inline void scatter(S1 *array, EntryType S1::* member1,
                const IndexType &indexes, const Mask &mask) const {
            VectorHelperSize<T>::scatter(*this, indexes, mask.shiftMask(), array, member1);
        }
        template<typename S1, typename S2> inline void scatter(S1 *array, S2 S1::* member1,
                EntryType S2::* member2, const IndexType &indexes) const {
            VectorHelperSize<T>::scatter(*this, indexes, array, member1, member2);
        }
        template<typename S1, typename S2> inline void scatter(S1 *array, S2 S1::* member1,
                EntryType S2::* member2, const IndexType &indexes, const Mask &mask) const {
            VectorHelperSize<T>::scatter(*this, indexes, mask.shiftMask(), array, member1, member2);
        }

        //prefix
        inline Vector &operator++() { data() = VectorHelper<T>::add(data(), VectorHelper<T>::one()); return *this; }
        //postfix
        inline Vector operator++(int) { const Vector<T> r = *this; data() = VectorHelper<T>::add(data(), VectorHelper<T>::one()); return r; }

        inline EntryType operator[](int index) const {
            return Base::d.m(index);
        }

#define OP1(fun) \
        inline Vector fun() const { return Vector<T>(VectorHelper<T>::fun(data())); } \
        inline Vector &fun##_eq() { data() = VectorHelper<T>::fun(data()); return *this; }
        OP1(sqrt)
        OP1(abs)
#undef OP1

#define OP(symbol, fun) \
        inline Vector &operator symbol##=(const Vector<T> &x) { data() = VectorHelper<T>::fun(data(), x.data()); return *this; } \
        inline Vector operator symbol(const Vector<T> &x) const { return Vector<T>(VectorHelper<T>::fun(data(), x.data())); }

        OP(+, add)
        OP(-, sub)
        OP(*, mul)
        OP(/, div)
        OP(|, or_)
        OP(&, and_)
        OP(^, xor_)
#undef OP
#define OPcmp(symbol, fun) \
        inline Mask operator symbol(const Vector<T> &x) const { return VectorHelper<T>::fun(data(), x.data()); }

        OPcmp(==, cmpeq)
        OPcmp(!=, cmpneq)
        OPcmp(>=, cmpnlt)
        OPcmp(>, cmpnle)
        OPcmp(<, cmplt)
        OPcmp(<=, cmple)
#undef OPcmp

        inline void multiplyAndAdd(const Vector<T> &factor, const Vector<T> &summand) {
            VectorHelper<T>::multiplyAndAdd(data(), factor, summand);
        }

        inline void assign( const Vector<T> &v, const Mask &mask ) {
            const VectorType k = mm128_reinterpret_cast<VectorType>(mask.data());
            data() = VectorHelper<VectorType>::blend(data(), v.data(), k);
        }

        template<typename T2> inline Vector<T2> staticCast() const { return StaticCastHelper<T, T2>::cast(data()); }
        template<typename T2> inline Vector<T2> reinterpretCast() const { return ReinterpretCastHelper<T, T2>::cast(data()); }

        inline WriteMaskedVector<T> operator()(const Mask &k) { return WriteMaskedVector<T>(this, k); }

        /**
         * \return \p true  This vector was completely filled. m2 might be 0 or != 0. You still have
         *                  to test this.
         *         \p false This vector was not completely filled. m2 is all 0.
         */
        //inline bool pack(Mask &m1, Vector<T> &v2, Mask &m2) {
            //return VectorHelper<T>::pack(data(), m1.data, v2.data(), m2.data);
        //}

        inline VectorType &data() { return Base::data(); }
        inline const VectorType &data() const { return Base::data(); }

        inline EntryType min() const { return VectorHelper<T>::min(data()); }
};

template<typename T> class SwizzledVector : public Vector<T> {};

template<typename T> inline Vector<T> operator+(const typename Vector<T>::EntryType &x, const Vector<T> &v) { return v.operator+(x); }
template<typename T> inline Vector<T> operator*(const typename Vector<T>::EntryType &x, const Vector<T> &v) { return v.operator*(x); }
template<typename T> inline Vector<T> operator-(const typename Vector<T>::EntryType &x, const Vector<T> &v) { return Vector<T>(x) - v; }
template<typename T> inline Vector<T> operator/(const typename Vector<T>::EntryType &x, const Vector<T> &v) { return Vector<T>(x) / v; }
template<typename T> inline typename Vector<T>::Mask  operator< (const typename Vector<T>::EntryType &x, const Vector<T> &v) { return Vector<T>(x) <  v; }
template<typename T> inline typename Vector<T>::Mask  operator<=(const typename Vector<T>::EntryType &x, const Vector<T> &v) { return Vector<T>(x) <= v; }
template<typename T> inline typename Vector<T>::Mask  operator> (const typename Vector<T>::EntryType &x, const Vector<T> &v) { return Vector<T>(x) >  v; }
template<typename T> inline typename Vector<T>::Mask  operator>=(const typename Vector<T>::EntryType &x, const Vector<T> &v) { return Vector<T>(x) >= v; }
template<typename T> inline typename Vector<T>::Mask  operator==(const typename Vector<T>::EntryType &x, const Vector<T> &v) { return Vector<T>(x) == v; }
template<typename T> inline typename Vector<T>::Mask  operator!=(const typename Vector<T>::EntryType &x, const Vector<T> &v) { return Vector<T>(x) != v; }

#define OP_IMPL(T, symbol, fun) \
  template<> inline Vector<T> &VectorBase<T>::operator symbol##=(const Vector<T> &x) { d.v() = VectorHelper<T>::fun(d.v(), x.d.v()); return *static_cast<Vector<T> *>(this); } \
  template<> inline Vector<T>  VectorBase<T>::operator symbol(const Vector<T> &x) const { return Vector<T>(VectorHelper<T>::fun(d.v(), x.d.v())); }
  OP_IMPL(int, &, and_)
  OP_IMPL(int, |, or_)
  OP_IMPL(int, ^, xor_)
  OP_IMPL(int, <<, sll)
  OP_IMPL(int, >>, srl)
  OP_IMPL(unsigned int, &, and_)
  OP_IMPL(unsigned int, |, or_)
  OP_IMPL(unsigned int, ^, xor_)
  OP_IMPL(unsigned int, <<, sll)
  OP_IMPL(unsigned int, >>, srl)
  OP_IMPL(short, &, and_)
  OP_IMPL(short, |, or_)
  OP_IMPL(short, ^, xor_)
  OP_IMPL(short, <<, sll)
  OP_IMPL(short, >>, srl)
  OP_IMPL(unsigned short, &, and_)
  OP_IMPL(unsigned short, |, or_)
  OP_IMPL(unsigned short, ^, xor_)
  OP_IMPL(unsigned short, <<, sll)
  OP_IMPL(unsigned short, >>, srl)
#undef OP_IMPL
#undef OP_IMPL2

  template<typename T> static inline Vector<T> min  (const Vector<T> &x, const Vector<T> &y) { return VectorHelper<T>::min(x.data(), y.data()); }
  template<typename T> static inline Vector<T> max  (const Vector<T> &x, const Vector<T> &y) { return VectorHelper<T>::max(x.data(), y.data()); }
  template<typename T> static inline Vector<T> min  (const Vector<T> &x, const typename Vector<T>::EntryType &y) { return min(x.data(), Vector<T>(y).data()); }
  template<typename T> static inline Vector<T> max  (const Vector<T> &x, const typename Vector<T>::EntryType &y) { return max(x.data(), Vector<T>(y).data()); }
  template<typename T> static inline Vector<T> min  (const typename Vector<T>::EntryType &x, const Vector<T> &y) { return min(Vector<T>(x).data(), y.data()); }
  template<typename T> static inline Vector<T> max  (const typename Vector<T>::EntryType &x, const Vector<T> &y) { return max(Vector<T>(x).data(), y.data()); }
  template<typename T> static inline Vector<T> sqrt (const Vector<T> &x) { return VectorHelper<T>::sqrt(x.data()); }
  template<typename T> static inline Vector<T> rsqrt(const Vector<T> &x) { return VectorHelper<T>::rsqrt(x.data()); }
  template<typename T> static inline Vector<T> abs  (const Vector<T> &x) { return VectorHelper<T>::abs(x.data()); }
  template<typename T> static inline Vector<T> sin  (const Vector<T> &x) { return VectorHelper<T>::sin(x.data()); }
  template<typename T> static inline Vector<T> cos  (const Vector<T> &x) { return VectorHelper<T>::cos(x.data()); }
  template<typename T> static inline Vector<T> log  (const Vector<T> &x) { return VectorHelper<T>::log(x.data()); }
  template<typename T> static inline Vector<T> log10(const Vector<T> &x) { return VectorHelper<T>::log10(x.data()); }

  template<typename T> static inline typename Vector<T>::Mask isfinite(const Vector<T> &x) { return VectorHelper<T>::isFinite(x.data()); }
  template<typename T> static inline typename Vector<T>::Mask isnan(const Vector<T> &x) { return VectorHelper<T>::isNaN(x.data()); }
#undef STORE_VECTOR
} // namespace SSE

#include "undomacros.h"

#endif // SSE_VECTOR_H
