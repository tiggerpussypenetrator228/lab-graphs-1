#include "profile.hpp"

#include <iostream>
#include <cstdlib>

#include <fstream>

#include "btree.hpp"

// Генерирует бинарное дерево. maxLeaves - максимальное количество элементов.
BinaryTree<int>* GenerateTree(int maxLeaves)
{
	// Установка сида для рандомных значений лепестков.
	srand(time(NULL));

	BinaryTree<int>* result = nullptr;

	// Очередь на генерацию.
	std::queue<leaf_generation_data_t<int>> toGenerate = {};
	toGenerate.push({ &result, nullptr, TreeDirection::ROOT });

	int leavesGenerated = 0;

	// Пока есть лепестки в очереди на генерацию...
	while (toGenerate.size() > 0)
	{
		// Получить данные о запросе на генерацию.
		const leaf_generation_data_t<int>& leafData = toGenerate.front();

		// Создать лепесток по этим данным со случайным значением.
		int leafValue = rand() % 255;
		(*leafData.output) = new BinaryLeaf<int>(leafValue);
		
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

		// Проверяем лимит лепестков.
		leavesGenerated++;
		if (leavesGenerated >= maxLeaves)
		{
			break;
		}

		// Добавляем в очередь на генерацию левый и правый будущих потомков созданного лепестка.
		toGenerate.push({ (*leafData.output)->GetRightChild(), (*leafData.output), TreeDirection::RIGHT });
		toGenerate.push({ (*leafData.output)->GetLeftChild(), (*leafData.output), TreeDirection::LEFT });

		// Удаляем из очереди на генерацию данные о созданном лепесток.
		toGenerate.pop();
	}

	return result;
}

int main(int argc, const char** argv)
{
	// Открываем поток ввода для файла tree.bt
	std::ifstream input = std::ifstream("btree.bt");

	// Поток вывода пока что не открыт, но объявлен.
	std::ofstream output;

	BinaryTree<int>* tree = nullptr;

	if (input.is_open())
	{
		// Десериализация.

		// Запускаем профилизацию памяти и времени.
		profile::StartMemoryProfiling();
		profile::StartTimeProfiling();

		// Десериализацией подгружаем дерево из потока ввода.
		tree = new BinaryTree<int>();
		BinaryTree<int>::Deserialize(input, &tree, [](const std::string& serialized) -> int {
			// Это лямбда обработки строкового значения. Тут мы просто преобразуем строковое число в int.
			return std::stoi(serialized);
		});

		// Завершаем профилизацию памяти и времени.
		profile::EndTimeProfiling();
		profile::EndMemoryProfiling();

		// Выводим информацию, полученную за время профилизации.
		std::cout << "1. Deserialization (loading from file) took " << profile::GetProfiledTime().count() << " microseconds." << std::endl;
		std::cout << "\t with " << profile::GetProfiledMemory() << " bytes of memory allocated in total" << std::endl << std::endl;

		input.close();
	}
	else
	{
		// Генерация.
		
		int maxLeaves = 0;

		// Запрашиваем у пользователя количество лепестков.
		std::cout << "Enter max amount of leaves: " << std::endl;
		std::cin >> maxLeaves;

		profile::StartMemoryProfiling();
		profile::StartTimeProfiling();

		// Генерируем дерево.
		tree = GenerateTree(maxLeaves);

		profile::EndTimeProfiling();
		profile::EndMemoryProfiling();

		std::cout << "1. Generation took " << profile::GetProfiledTime().count() << " microseconds." << std::endl;
		std::cout << "\t with " << profile::GetProfiledMemory() << " bytes of memory allocated in total" << std::endl << std::endl;

		// Открываем поток вывода, так как после генерации дерево нужно вывести в файл.
		output = std::ofstream("btree.bt");
	}

	BinaryTree<int>* maxRatioSubtree = nullptr;
	double maxRatio = 0.0;

	BinaryTree<int>* minRatioSubtree = nullptr;
	double minRatio = 99999999.0;

	// Нахождение необходимых отношений.

	profile::StartMemoryProfiling();
	profile::StartTimeProfiling();

	tree->GetMinMaxWeightSumChildrenRatio(minRatio, minRatioSubtree, maxRatio, maxRatioSubtree);

	profile::EndTimeProfiling();
	profile::EndMemoryProfiling();

	std::cout << "2. Search took " << profile::GetProfiledTime().count() << " microseconds." << std::endl;
	std::cout << "\t with " << profile::GetProfiledMemory() << " bytes of memory allocated in total" << std::endl << std::endl;

	// Если поток вывода открыт, сериализируем дерево.
	if (output.is_open())
	{
		// Сериализация.

		profile::StartMemoryProfiling();
		profile::StartTimeProfiling();

		tree->Serialize(output);

		profile::EndTimeProfiling();
		profile::EndMemoryProfiling();

		std::cout << "3. Serialization (writing to file) took " << profile::GetProfiledTime().count() << " microseconds." << std::endl;
		std::cout << "\t with " << profile::GetProfiledMemory() << " bytes of memory allocated in total" << std::endl << std::endl;

		output.close();
	}

	// Сериализируем основное дерево, его размер, а так же найденные отношения и поддеревья в поток cout.
	// Таким образом сериализованные данные выведутся в консоль.

	std::cout << tree->GetByteSize() << " bytes used by tree" << std::endl;
	std::cout << std::endl << "Tree: " << std::endl;

	tree->Serialize(std::cout, 6, true);

	std::cout << std::endl << "Minimum ratio subtree: " << std::endl;
	std::cout << minRatio << " ratio; Tree: " << std::endl;
	minRatioSubtree->Serialize(std::cout, 6, true);

	std::cout << std::endl << "Maximum ratio subtree: " << std::endl;
	std::cout << maxRatio << " ratio; Tree: " << std::endl;
	maxRatioSubtree->Serialize(std::cout, 6, true);

	return 0;
}
