// skeleton.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

#include <linux/moduleparam.h>	// needed for module parameters

#include <linux/slab.h>        // needed for kzalloc
#include <linux/list.h>        // needed for linked list
#include <linux/string.h>      // needed for string functions

static char* text = "This is the best module ever made!";
module_param(text, charp, 0664);
static int  elements = 0;
module_param(elements, int, 0);

struct element {
    char text[100];
    int id;
    struct list_head node;
};

static LIST_HEAD (element_list);

static int __init best_init(void)
{
    int i;

	pr_info ("Linux module 01 best loaded\n");
	pr_debug ("  text: %s\n  elements: %d\n", text, elements);

    for (i = 0; i < elements; i++) {
        struct element *e = kzalloc(sizeof(struct element), GFP_KERNEL);
        if (e == NULL) {
            pr_err("Unable to allocate memory for element %d", i);
            return -ENOMEM;
        } else {
            e->id = i;
            strncpy(e->text, text, 99);
            list_add_tail(&e->node, &element_list);
        }
    }

	return 0;
}

static void __exit best_exit(void)
{
    struct element *e;
    int nb_elements = 0;

    list_for_each_entry(e, &element_list, node) {
        pr_info("Element %d: %s\n", e->id, e->text);
        nb_elements++;
    }

    pr_info("Number of elements: %d\n", nb_elements);

    while(!list_empty(&element_list)) {
        e = list_entry(element_list.next, struct element, node);
        list_del(&e->node);
        kfree(e);
    }

    pr_info("All elements deleted (%d out of %d)\n", nb_elements, elements);
	pr_info ("Linux module best unloaded\n");
}

module_init (best_init);
module_exit (best_exit);

MODULE_AUTHOR ("Samy Francelet <samy.francelet@ik.me>");
MODULE_DESCRIPTION ("Module best");
MODULE_LICENSE ("GPL");

