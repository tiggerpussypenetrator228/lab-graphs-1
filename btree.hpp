#pragma once

#include <algorithm>
#include <queue>
#include <functional>
#include <string>

// Объявление лепестка наперёд.
template<typename T>
class BinaryLeaf;

// Объявление дерева наперёд.
template<typename T>
using BinaryTree = BinaryLeaf<T>;

/* 
	Здесь можно было бы использовать enum class, явно указывая подтип uint8_t, но судя по всему
	enum class не унаследует размерность от подтипа и всегда занимает 4 байта. Поэтому был создан
	тип treedir_t, чтобы явно указать компилятору, что направление лепестка не должно занимать больше 1 байта.
*/
typedef uint8_t treedir_t;
namespace TreeDirection
{
	treedir_t ROOT = 0;

	treedir_t LEFT = 1;
	treedir_t RIGHT = 2;
}

// Данные, используемые для генерации и десериализации лепестка.
template<typename T>
struct leaf_generation_data_t
{
	// Указатель на место, куда должен будет поместиться указатель на сгенерированный лепесток в будущем.
	BinaryLeaf<T>** output;

	// Родитель лепестка, который необходимо сгенерировать.
	BinaryLeaf<T>* parent;

	// Направление лепестка.
	treedir_t direction;
};

// Имплементация лепестка (и дерева).
template<typename T>
class BinaryLeaf
{
public:
	/*
		Этот callback используется в итерации по дереву. Его задаёт программист, чтобы
		указать функционал, который должен исполнится на каждый лепесток дерева.

		Если эта лямбда вернёт true, то итерация сразу же прекратится, и цикл не перейдёт к
		следующему потомку. Таким образом программист может в любое время вызвать своеобразный "break".

		"continue" же будет возвращением false, так как при возвращении false лямбда не продолжает исполнение
		и метод итерации просто переходит на следующего потомка.
	*/
	using walk_callback_t = std::function<bool(BinaryLeaf<T>*)>;

	/*
		Эта лямбда используется в десериализации дерева. Её задача - превратить строковое
		значение лепестка в исходное состояние. Например, если есть строка "123", то эта лямбда
		должна превратить её в int значение 123, учитывая, что T лепестка равняется int.
	*/
	using deserializer_t = std::function<T(const std::string&)>;
private:
	// Значение лепестка.
	T mValue;

	// Глубина лепестка.
	uint16_t mDepth;

	// Направление лепестка.
	treedir_t mDirection;

	// Потомки лепестка - левый и правый.
	BinaryLeaf<T>* mRight;
	BinaryLeaf<T>* mLeft;
public:
	// Стандартный конструктор лепестка.
	BinaryLeaf()
	{
		mValue = T();

		mDepth = 0;
		mDirection = TreeDirection::ROOT;

		mRight = mLeft = nullptr;
	}

	// Конструктор лепестка, задающий изначальное значение.
	BinaryLeaf(T value)
	{
		mValue = value;

		mDepth = 0;
		mDirection = TreeDirection::ROOT;

		mRight = mLeft = nullptr;
	}

	// Деструктор лепестка, уничтожающий всех потомков в цикле. Метод Walk описывается чуть ниже.
	~BinaryLeaf()
	{
		// Проходимся по всем потомкам и удаляем их, не включая себя.
		Walk([](BinaryLeaf<T>* leaf) -> bool {
			delete leaf;

			return false;
		}, false);
	}
public:
	// Получение размера всего дерева в байтах.
	size_t GetByteSize()
	{
		size_t result = 0;

		// Проходимся по всем потомкам и добавляем их размер к сумме, включая себя.
		Walk([&](BinaryLeaf<T>* leaf) -> bool {
			result += sizeof(*leaf);

			return false;
		});

		return result;
	}
public:
	/*
		Этот метод использует способ очереди для итерации по всем
		потомкам текущего лепестка (или корня дерева, так как он тоже является лепестком).

		Callback "walker" вызывается на каждого потомка этого лепестка.

		Если флаг includeSelf установлен в false, то лямбда walker не будет вызвана
		на корень, то есть на лепесток, который вызвал метод Walk. Это нужно, чтобы пройтись
		только по потомкам лепестка, не включая сам лепесток.
	*/
	void Walk(walk_callback_t walker, bool includeSelf = true)
	{
		// Очередь лепестков для итерации.
		std::queue<BinaryLeaf<T>*> collected = {};

		/*
			Если надо добавить текущий лепесток, то добавляем this в очередь.
			Иначе добавляем только потомков mLeft и mRight.
		*/
		if (includeSelf)
		{
			collected.push(this);
		}
		else
		{
			if (mLeft != nullptr)
			{
				collected.push(mLeft);
			}

			if (mRight != nullptr)
			{
				collected.push(mRight);
			}
		}

		// Пока в очереди есть лепестки...
		while (collected.size() > 0)
		{
			// Получаем первый на очереди лепесток.
			BinaryLeaf<T>* leaf = collected.front();
			collected.pop();

			// Добавляем левого и правого потомков полученного лепестка в очередь, если они есть.

			if (leaf->mRight != nullptr)
			{
				collected.push(leaf->mRight);
			}

			if (leaf->mLeft != nullptr)
			{
				collected.push(leaf->mLeft);
			}

			// Вызываем переданную в Walk лямбду и передаём туда полученный лепесток. Ожидаем, чтобы она вернула bool.
			bool shouldStop = walker(leaf);

			// Если лямбда вернула true, то прекращаем проходиться по очереди.
			if (shouldStop)
			{
				break;
			}
		}
	}
public:
	/* 
		Методы установки потомков лепестка.
		При их вызове устанавливается соответсвующее направление и глубина.
	*/

	void SetLeftChild(BinaryLeaf<T>* leaf)
	{
		mLeft = leaf;

		mLeft->mDepth = mDepth + 1;
		mLeft->mDirection = TreeDirection::LEFT;
	}

	void SetRightChild(BinaryLeaf<T>* leaf)
	{
		mRight = leaf;

		mRight->mDepth = mDepth + 1;
		mRight->mDirection = TreeDirection::RIGHT;
	}
	
	// Получение потомков соответственно.

	BinaryLeaf<T>* GetLeftChild() const
	{
		return mLeft;
	}

	BinaryLeaf<T>* GetRightChild() const
	{
		return mRight;
	}

	/* 
		Получение указателей на поля mLeft и mRight данного лепестка.
		Это нужно, чтобы записать сгенерированные лепестки в данные поля в будущем.
	*/

	BinaryLeaf<T>** GetLeftChild()
	{
		return &mLeft;
	}

	BinaryLeaf<T>** GetRightChild()
	{
		return &mRight;
	}

	// Установка и получение значения этого лепестка.

	T GetValue()
	{
		return mValue;
	}

	void SetValue(T value)
	{
		mValue = value;
	}

	// Получение глубины этого лепестка.

	uint16_t GetDepth()
	{
		return mDepth;
	}
public:
	// Получаем отношение (сумма весов / количество потомков) для данного лепестка (или дерева).
	double GetWeightSumChildrenRatio()
	{
		// Количество потомков данного лепестка.
		int children = 0;

		// Сумма весов. Инициализируем весом текущего лепестка.
		int weightSum = (mDepth * mValue);

		/*
			Проходимся по всем потомкам данного лепестка.
			С каждым вызовом увеличиваем количество потомков и добавляем вес потомка к сумме весов.
		*/
		Walk([&](BinaryLeaf<T>* leaf) -> bool {
			children++;

			weightSum += (leaf->mDepth * leaf->mValue);

			return false;
		}, false);

		// На 0 делить нельзя. Убеждаемся, что количество потомков хотя бы равняется 1.
		children = std::max(1, children);

		// Кастуем к числу с плавающей точкой и делим, затем возвращаем полученное отношение.
		return static_cast<double>(weightSum) / static_cast<double>(children);
	}

	/*
		Этот метод просто проходится по всем потомкам, включая текущий лепесток, и находит максимальное и минимальное отношение.
		
		Минимальное отношение записывается по ссылке outputMin, максимальное - outputMax.
		Соответствующие им поддеревья записываются по ссылкам outputMinHolder и outputMaxHolder.
	*/
	void GetMinMaxWeightSumChildrenRatio(double& outputMin, BinaryLeaf<T>*& outputMinHolder, double& outputMax, BinaryLeaf<T>*& outputMaxHolder)
	{
		Walk([&](BinaryLeaf<T>* leaf) -> bool {
			double ratio = leaf->GetWeightSumChildrenRatio();

			if (ratio < outputMin)
			{
				outputMin = ratio;
				outputMinHolder = leaf;
			}
			
			if (ratio > outputMax)
			{
				outputMax = ratio;
				outputMaxHolder = leaf;
			}

			return false;
		});
	}
public:
	/*
		Метод сериализации. Приводит дерево в вид, который можно либо хранить в файле, либо вывести в консоль.

		stream - поток вывода, куда дерево будет сериализовываться. Может быть как cout, так и ofstream.

		Следующие аргументы не стоит передавать для хранения в файле, так как десериализатор их не обрабатывает:
		skipDeep - при достижении данной глубины сериализация прекратится. Может быть -1 в случае если ограничение не требуется.
		pretty - включить табуляцию.
	*/
	void Serialize(std::ostream& stream, uint16_t skipDeep = -1, bool pretty = false)
	{
		Walk([&](BinaryLeaf<T>* leaf) -> bool {
			// "Красивизация" дерева.
			if (pretty)
			{
				// Максимальное количество табов - 32.
				uint16_t tabDepth = (leaf->mDepth < 32) ? leaf->mDepth : 32;

				// Левые лепестки будут чуть ближе к левому краю, чтобы их различать было легче.
				if (leaf->mDirection == TreeDirection::LEFT)
				{
					tabDepth -= 1;
				}

				// Вывод табов.
				for (uint16_t t = 0; t < tabDepth; t++)
				{
					stream << "\t";
				}

				// Вывод глубины и двоеточия.
				stream << leaf->mDepth << ": ";
			}
			
			// Вывод значения лепестка и перенос на следующую строку.
			stream << leaf->mValue << std::endl;

			// Если skipDeep включен и мы его достигли по глубине, то не продолжать дальше выводить лепестки.
			if (skipDeep != -1 && leaf->mDepth > skipDeep)
			{
				stream << "..." << std::endl;

				return true;
			}

			return false;
		});
	}

	/*
		Метод десериализации (статический). Из дерева в файле создаёт дерево в коде и записывает его по указателю output.

		stream - поток ввода. может быть как cin, так и ifstream.
		valueDeserializer - десериализатор строковых значений в T данного лепестка.
	*/
	static void Deserialize(std::istream& stream, BinaryLeaf<T>** output, deserializer_t valueDeserializer)
	{
		// Очередь лепестков на популяцию.
		std::queue<leaf_generation_data_t<T>> toPopulate = {};
		toPopulate.push({ output, nullptr, TreeDirection::ROOT });

		// Текущая строка в потоке.
		std::string curline = "";

		// Пока в потоке есть данные и пока очередь на популяцию не пустая...
		while (stream.good() && toPopulate.size() > 0)
		{
			// Получаем текущую строку и проверяем её на пустоту.
			std::getline(stream, curline);
			if (curline.size() <= 0)
			{
				continue;
			}

			// Преобразовываем строку в T значение лепестка.
			T value = valueDeserializer(curline);

			// Создаём лепесток с преобразованным значением.
			const leaf_generation_data_t<T>& leafData = toPopulate.front();
			(*leafData.output) = new BinaryLeaf<T>(value);

			// Устанавливаем иерархию, направление лепестка и его глубину.
			if (leafData.parent != nullptr)
			{
				if (leafData.direction == TreeDirection::LEFT)
				{
					leafData.parent->SetLeftChild((*leafData.output));
				}
				else if (leafData.direction == TreeDirection::RIGHT)
				{
					leafData.parent->SetRightChild((*leafData.output));
				}
			}

			// Добавляем в очередь на популяцию левый и правый будущих потомков созданного лепестка.
			toPopulate.push({ (*leafData.output)->GetRightChild(), (*leafData.output), TreeDirection::RIGHT });
			toPopulate.push({ (*leafData.output)->GetLeftChild(), (*leafData.output), TreeDirection::LEFT });

			// Удаляем из очереди на популяцию созданный лепесток.
			toPopulate.pop();
		}
	}
};
