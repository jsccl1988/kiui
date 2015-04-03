//  Copyright (c) 2015 Hugo Amiard hugo.amiard@laposte.net
//  This software is provided 'as-is' under the zlib License, see the LICENSE.txt file.
//  This notice and the license may not be removed or altered from any source distribution.

#ifndef MK_TYPED_H_INCLUDED
#define MK_TYPED_H_INCLUDED

/* mk */
#include <Object/mkObjectForward.h>
#include <Object/mkType.h>
#include <Object/mkObject.h>

namespace mk
{
	template <class T>
	inline Type* typecls()
	{
		return T::cls();
	}

	template <> inline Type* typecls<void>() { return nullptr; }

	template <class T, class I = void>
	class Typed
	{
	public:
		static inline Type* cls() { return &sType; }

	private:
		static Type sType;
	};

	template <class T, class I>
	Type Typed<T, I>::sType(typecls<I>());

	template <class T>
	T* upcast(TypeObject* object)
	{
		if(object->type()->upcast(typecls<T>()))
			return static_cast<T*>(object);

		return 0;
	}

	template <class T>
	T* downcast(TypeObject* object)
	{
		if(object->type() == typecls<T>())
			return static_cast<T*>(object);

		return 0;
	}

	template <> inline Type* typecls<bool>() { return Typed<bool>::cls(); }
	template <> inline Type* typecls<int>() { return Typed<int>::cls(); }
	template <> inline Type* typecls<unsigned int>() { return Typed<unsigned int>::cls(); }
	template <> inline Type* typecls<float>() { return Typed<float>::cls(); }
	template <> inline Type* typecls<double>() { return Typed<double>::cls(); }
	template <> inline Type* typecls<string>() { return Typed<string>::cls(); }
	
	template <> inline Type* typecls<std::vector<float>>() { return Typed<std::vector<float>>::cls(); }
	template <> inline Type* typecls<std::vector<string>>() { return Typed<std::vector<string>>::cls(); }

	template <> inline Type* typecls<std::vector<Type*>>() { return Typed<std::vector<Type*>>::cls(); }
	template <> inline Type* typecls<std::vector<Object*>>() { return Typed<std::vector<Object*>>::cls(); }

	template class MK_OBJECT_EXPORT Typed<bool>;
	template class MK_OBJECT_EXPORT Typed<int>;
	template class MK_OBJECT_EXPORT Typed<unsigned int>;
	template class MK_OBJECT_EXPORT Typed<float>;
	template class MK_OBJECT_EXPORT Typed<double>;
	template class MK_OBJECT_EXPORT Typed<string>;

	template class MK_OBJECT_EXPORT Typed<std::vector<float>>;
	template class MK_OBJECT_EXPORT Typed<std::vector<string>>;

	template class MK_OBJECT_EXPORT Typed<std::vector<Type*>>;
	template class MK_OBJECT_EXPORT Typed<std::vector<Object*>>;
}

#endif // MK_TYPED_INCLUDED