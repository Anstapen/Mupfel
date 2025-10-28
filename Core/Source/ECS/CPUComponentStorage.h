#pragma once
#include "ComponentStorage.h"
#include <vector>

template <typename T>
class CPUComponentStorage :
    public Mupfel::ComponentStorage<T>
{
public:
	CPUComponentStorage() = default;
	virtual ~CPUComponentStorage() = default;
	size_t size() const override;
	void push_back(const T& item) override;
	void push_back(T&& item) override;
	void pop_back() override;
	const T& Read(size_t index) const override;
	T& Read(size_t index) override;
	void Write(size_t pos, const T& val) override;
	void clear() override;
	uint32_t Id() const override;
private:
	std::vector<T> vector;
};

template<typename T>
inline size_t CPUComponentStorage<T>::size() const
{
	return vector.size();
}

template<typename T>
inline void CPUComponentStorage<T>::push_back(const T& item)
{
	vector.push_back(item);
}

template<typename T>
inline void CPUComponentStorage<T>::push_back(T&& item)
{
	vector.push_back(item);
}

template<typename T>
inline void CPUComponentStorage<T>::pop_back()
{
	vector.pop_back();
}

template<typename T>
inline const T& CPUComponentStorage<T>::Read(size_t index) const
{
	return vector[index];
}

template<typename T>
inline T& CPUComponentStorage<T>::Read(size_t index)
{
	return vector[index];
}

template<typename T>
inline void CPUComponentStorage<T>::Write(size_t pos, const T& val)
{
	vector[pos] = val;
}

template<typename T>
inline void CPUComponentStorage<T>::clear()
{
	vector.clear();
}

template<typename T>
inline uint32_t CPUComponentStorage<T>::Id() const
{
	return 0;
}
