
#define EnumValue(name, number) name = number,

enum class EnumName : EnumType {
	EnumValues
};

#undef EnumValue
#define EnumValue(name, number) case EnumName::name: return #name; break;

inline char *getStrFromEnumValue(EnumName tag_value) { 
	switch(tag_value) {
		EnumValues
		default: return formatString("Unrecognised value (%d)", (u32)tag_value); break;
	}
}

#undef EnumValue
#undef EnumName
#undef EnumType
#undef EnumValues