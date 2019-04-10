#ifndef REFLECTION_H
#define REFLECTION_H

#include <engine/std.h>
#include <engine/math.h>

namespace Reflection {
	struct Type {
		const char *name;
		const char *format;
		size_t size;
		bool is_primitive;
	};

	struct Field {
		const Type *type;
		const char *name;
		size_t offset;

		template<typename T>
		void setValue(void *instance, T value) {
			u8 *walker = (u8 *)instance;
			walker += offset;
			memcpy(walker, &value, type->size);
		}

		template<typename T>
		T getValue(void *intstance) {
			u8 *walker (u8 *)instance;
			walker += offset;
			T result = *(T *)walker;
			return result;
		}
	};

	struct Class : public Type {
		Field *fields;
		u32 field_count;

		void print();
	};

	template<typename T>
	Class *getStruct();

	template<typename T>
	Class *getStruct(const T *obj) {
		return getStruct<T>();
	}

	template<typename T>
	void printStruct(const T *obj) {
		Class *class_type = getStruct<T>();
		class_type->print();
	}
}

#endif // REFLECTION_H