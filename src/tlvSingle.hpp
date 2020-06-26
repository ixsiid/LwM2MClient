#pragma once

#include "tlv.hpp"

namespace LwM2M {

class TLVSingle : public TLV {
    public:
	TLVSingle(uint16_t id, DataType type, void* value);
	TLVSingle(uint16_t id, DataType type, TLVCallback value);

	~TLVSingle() override;

	/// 整数型の場合 single->add((void *)3);
	///
	/// 構造体をつかう場合
	/// pointer_t p = {length, buffer};
	/// single->add(&p);
	///
	/// 浮動小数点はdouble非対応
	void add(const void* value) override;
	bool hasValue() override;

    private:
	bool _hasValue;
};

}  // namespace LwM2M
