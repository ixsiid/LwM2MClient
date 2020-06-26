#pragma once

#include "list.hpp"
#include "tlv.hpp"

namespace LwM2M {

class TLVMulti : public TLV {
    public:
	TLVMulti(uint16_t id, DataType type, void* values);
	TLVMulti(uint16_t id, DataType type, TLVCallback values);

	~TLVMulti() override;

	/// 整数型の場合 single->add((void *)3);
	///
	/// 構造体をつかう場合
	/// pointer_t p = {length, buffer};
	/// single->add(&p);
	///
	/// 浮動小数点はdouble非対応
	void add(const void* value) override;
	bool hasValue() override;

	void clear();
	void set(const void* value);

    private:
	List* list;
};

}  // namespace LwM2M
