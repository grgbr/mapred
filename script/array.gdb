source common.gdb

define print_array_fixed
	printf "[%p]", &($arg0)
	echo $arg0:
	printf " nr = %u\n", ($arg0).arr_nr

	set $node = ($arg0).arr_items
	set $n = 0
	while $n < ($arg0).arr_nr
		printf "\t[%p]%u: ", $node, $n
		output *(($arg1)($node))
		echo \n
		set $node = $node + sizeof($arg1)
		set $n = $n + 1
	end
	echo \n
end

document print_array_fixed
	print_array_fixed ARRAY_FIXED TYPE

	Print content of array_fixed passed in argument

	ARRAY_FIXED refers to the struct array_fixed reference to print
	TYPE refers to array_fixed items type
end

define print_bstree_fixed
	printf "[%p]", &($arg0)
	echo $arg0:
	printf " count = %u, nr = %u\n", ($arg0).bst_count, ($arg0).bst_nodes.arr_nr
	set $node = ($arg0).bst_nodes.arr_items
	set $n = 0
	while $n < ($arg0).bst_count
		printf "\t[%p]%u: ", $node, $n
		output *(($arg1)($node))
		echo \n
		set $node = $node + sizeof($arg1)
		set $n = $n + 1
	end
	echo \n
end

document print_bstree_fixed
	print_bstree_fixed BSTREE_FIXED TYPE

	Print content of bstree_fixed passed in argument

	BSTREE_FIXED refers to the struct bstree_fixed reference to print
	TYPE refers to bstree_fixed items type
end

define print_bheap_fixed
	printf "[%p]", &($arg0)
	echo $arg0:
	printf " node_size = %u, count = %u, nr = %u\n", ($arg0).node_size, ($arg0).bheap_tree.bst_count, ($arg0).bheap_tree.bst_nodes.arr_nr
	set $node = ($arg0).bheap_tree.bst_nodes.arr_items
	set $n = 0
	while $n < ($arg0).bheap_tree.bst_count
		printf "\t[%p]%u: ", $node, $n
		output *(($arg1)($node))
		echo \n
		set $node = $node + sizeof($arg1)
		set $n = $n + 1
	end
	echo \n
end

document print_bheap_fixed
	print_bheap_fixed BHEAP_FIXED TYPE

	Print content of bheap_fixed passed in argument

	BHEAP_FIXED refers to the struct bheap_fixed reference to print
	TYPE refers to bheap_fixed items type
end
