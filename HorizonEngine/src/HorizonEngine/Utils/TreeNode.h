#include <vector>

namespace Hzn {
	template <typename T>
	class TreeNode {
	public:
		int level;
		T item;
		std::vector<std::shared_ptr<TreeNode<T>>> nextNodes;
	public:
		TreeNode<T>();
		TreeNode<T>(int level, T item);
		~TreeNode<T>();

		void add(std::shared_ptr<TreeNode<T>> treeNode);
		void remove(std::shared_ptr<TreeNode<T>> treeNode);
	};

	template <typename T>
	TreeNode<T>::TreeNode() {
		level = 0;
		nextNodes = std::vector<std::shared_ptr<TreeNode<T>>>();
	}

	template <typename T>
	TreeNode<T>::TreeNode(int level, T item) {
		this.level = level;
		this.item = item;
		nextNodes = std::vector<std::shared_ptr<TreeNode<T>>>();
	}

	template <typename T>
	TreeNode<T>::~TreeNode() {

	}

	template <typename T>
	void TreeNode<T>::add(std::shared_ptr<TreeNode<T>> treeNode) {
		nextNodes.push_back(treeNode);
	}

	template <typename T>
	void TreeNode<T>::remove(std::shared_ptr<TreeNode<T>> treeNode) {
		for (int i = 0; i < nextNodes.size(); i++) {
			if (nextNodes.at(i)->item == treeNode->item) {
				nextNodes.erase(std::next(nextNodes.begin(), i));
				break;
			}
		}
	}
}