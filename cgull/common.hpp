// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

namespace CGull::guts
{
    template<typename _T> inline
    shared_data_ptr<_T>::shared_data_ptr(_T* from) noexcept
        : d(from)
    {
        if(d)
            d->_ref.ref();
    }

    template<typename _T> inline
    shared_data_ptr<_T>::shared_data_ptr(const shared_data_ptr<_T>& other)
        : d(other.d)
    {
        if(d)
            d->_ref.ref();
    }

    template<typename _T>
    template<typename _OtherT> inline
    shared_data_ptr<_T>::shared_data_ptr(const shared_data_ptr<_OtherT>& other)
        : d(static_cast< _T* >(other.data()))
    {
        if(d)
            d->_ref.ref();
    }

    template<typename _T> inline
    shared_data_ptr<_T>::shared_data_ptr(shared_data_ptr&& other) noexcept
        : d(other.d)
    {
        other.d = nullptr;
    }

    template<typename _T> inline
    shared_data_ptr<_T>::~shared_data_ptr()
    {
        if(d && !d->_ref.deref())
            delete d;
    }

    template<typename _T> inline
    shared_data_ptr<_T>& shared_data_ptr<_T>::operator=(const shared_data_ptr<_T>& other)
    {
        if(other.d != d)
        {
            if(other.d)
                other.d->_ref.ref();

            _T* old = d;
            d = other.d;

            if(old && !old->_ref.deref())
                delete old;
        }

        return *this;
    }

    template<typename _T> inline
    shared_data_ptr<_T>& shared_data_ptr<_T>::operator=(shared_data_ptr<_T>&& other) noexcept
    {
        std::swap(d, other.d);

        return *this;
    }

    template<typename _T> inline
    shared_data_ptr<_T>& shared_data_ptr<_T>::operator=(_T* other)
    {
        if(other != d)
        {
            if(other)
                other->_ref.ref();

            _T* old = d;
            d = other;

            if(old && !old->_ref.deref())
                delete old;
        }

        return *this;
    }

    template<typename _T> inline
    void shared_data_ptr<_T>::detach()
    {
        if(d && d->_ref.load() != 1)
        {
            _T* x = clone();
            x->_ref.ref();

            if(!d->_ref.deref())
                delete d;

            d = x;
        };
    }

    template<typename _T> inline
    void shared_data_ptr<_T>::reset()
    {
        if(d && !d->_ref.deref())
            delete d;

        d = nullptr;
    }

    template<typename _T> inline
    void shared_data_ptr<_T>::swap(shared_data_ptr &other) noexcept
    {
        std::swap(d, other.d);
    }

    template<typename _T> inline
    _T* shared_data_ptr<_T>::clone()
    {
        return new _T(*d);
    }
};
