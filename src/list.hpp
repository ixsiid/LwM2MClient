#ifndef __ARRAY_BUFFER_H
#define __ARRAY_BUFFER_H

#include <functional>

namespace LwM2M {
typedef struct {
	long id;
	void *next;
	void *data;
} Item;

class List {
    public:
	List();

	/// リストからデータを検索します。
	/// long id: データを識別する一意のID
	/// 返り値: 既にIDが存在したらデータのポインタ
	///                存在しない場合、nullptr
	void *find(long id);

	/// コールバック関数によってマッチしたアイテムを除去します。
	/// 除去されるのは最初の1つだけです。
	/// fruc: 除去する場合は trueを返す関数
	/// 返り値: 除去されたアイテムのデータポインタ
	void *removeAt(std::function<bool(void *data)> callback);

	/// リストにデータを追加します。
	/// long id: データを識別する一意のID
	/// void * data: 格納するデータへのポインタ
	/// 返り値: 既にIDが存在したら古いデータのポインタ
	///                存在しない場合、nullptr
	void *add(long id, void *data);

	/// リストからデータを削除します。
	/// long id: データを識別する一意のID
	/// 返り値: IDが存在したら、そのデータのポインタ
	///            存在しない場合、nullptr
	void *remove(long id);

	/// リストを掃引します。
	/// long id: コールバック関数
	void all(std::function<void(long id, void *data)> callback);

    private:
	Item *list;
};
}  // namespace LwM2M

#endif  // __ARRAY_BUFFER_H
