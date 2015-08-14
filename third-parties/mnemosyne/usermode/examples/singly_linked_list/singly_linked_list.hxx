/*!
 * \file
 * Defines an extremely simple linked-list class, with which we are able to play easily.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef LIST_HXX_QGJ6Z72Y
#define LIST_HXX_QGJ6Z72Y

#include <ostream>
#include <mnemosyne.h>
#include <malloc.h>

/*!
 * A simple, singly-linked list implementation. The goal here is an extremely
 * straightforward implementation with which we can play.
 */
template <class DataType>
class SinglyLinkedList
{
public:
	void *operator new(size_t  num_bytes) {
		void* result;
		MNEMOSYNE_ATOMIC result = pmalloc(num_bytes);
		return result;
	}

	/*!
	 * Constructs an empty list, having zero elements.
	 */
	inline SinglyLinkedList () :
		itsHeadNode(NULL),
		itsTailNode(NULL),
		itsLength(0)
	{	}
	
	/*!
	 * Appends an item to the list's tail.
	 * \param data is the value which is concatenated to the end of the list.
	 */
	__attribute__((tm_callable)) void append (DataType data);
	
	/*!
	 * Returns the length of the list.
	 */
	inline
	std::size_t length() const	{
		return itsLength;
	}
	
	template <class T>
	friend std::ostream& operator<< (std::ostream&, SinglyLinkedList<T>&);
	
	/*!
	 * The internal representation of list nodes.
	 */
	class Node
	{
	
	public:
		/*!
		 * Build a list node with the given data.
		 * \param data the object to store in this list node.
		 */
		__attribute__((tm_callable))
		inline
		Node (DataType& data) :
			itsData(data),
			itsNextNode(NULL)
		{	}
		
		friend class SinglyLinkedList<DataType>;
		
		template <class T>
		friend std::ostream& operator<< (std::ostream&, SinglyLinkedList<T>&);

	private:
		DataType itsData;   /*!< The data stored at this list node. */
		Node* itsNextNode;  /*!< The node following this, in the list. */
	};
	
private:
	
	Node* itsHeadNode;  /*!< The first linked node in the list. NULL if the list is empty. */
	Node* itsTailNode;  /*!< The last linked node in the list. NULL if the list is empty. */
	std::size_t itsLength;  /*!< The number of nodes in the list. */
};

#include "singly_linked_list.cxx"

#endif /* end of include guard: LIST_HXX_QGJ6Z72Y */
