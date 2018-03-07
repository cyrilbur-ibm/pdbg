#include <string.h>

#include "target.h"
#include "device.h"
#include "libpdbg.h"

struct pdbg_target *__pdbg_next_target(const char *class, struct pdbg_target *parent, struct pdbg_target *last)
{
	struct pdbg_target *next, *tmp;
	struct pdbg_target_class *target_class;

	if (class && !find_target_class(class))
		return NULL;

	target_class = require_target_class(class);

retry:
	/* No more targets left to check in this class */
	if ((last && last->class_link.next == &target_class->targets.n) ||
	    list_empty(&target_class->targets))
		return NULL;

	if (last)
		next = list_entry(last->class_link.next, struct pdbg_target, class_link);
	else
		if (!(next = list_top(&target_class->targets, struct pdbg_target, class_link)))
			return NULL;

	if (!parent)
		/* Parent is null, no need to check if this is a child */
		return next;
	else {
		/* Check if this target is a child of the given parent */
		for (tmp = next; tmp && next->dn->parent && tmp != parent; tmp = tmp->dn->parent->target) {}
		if (tmp == parent)
			return next;
		else {
			last = next;
			goto retry;
		}
	}
}

struct pdbg_target *__pdbg_next_child_target(struct pdbg_target *parent, struct pdbg_target *last)
{
	if (!parent || list_empty(&parent->dn->children))
		return NULL;

	if (!last)
		return list_top(&parent->dn->children, struct dt_node, list)->target;

	if (last->dn->list.next == &parent->dn->children.n)
		return NULL;

	return list_entry(last->dn->list.next, struct dt_node, list)->target;
}

enum pdbg_target_status pdbg_target_status(struct pdbg_target *target)
{
	struct dt_property *p;

	p = dt_find_property(target->dn, "status");
	if (!p)
		return PDBG_TARGET_ENABLED;

	if (!strcmp(p->prop, "enabled"))
		return PDBG_TARGET_ENABLED;
	else if (!strcmp(p->prop, "disabled"))
		return PDBG_TARGET_DISABLED;
	else if (!strcmp(p->prop, "hidden"))
		return PDBG_TARGET_HIDDEN;
	else if (!strcmp(p->prop, "nonexistant"))
		return PDBG_TARGET_NONEXISTANT;
	else
		assert(0);
}

void pdbg_enable_target(struct pdbg_target *target)
{
	struct dt_property *p;
	enum pdbg_target_status status = pdbg_target_status(target);

	if (status == PDBG_TARGET_ENABLED)
		return;

	p = dt_find_property(target->dn, "status");
	dt_del_property(target->dn, p);
}

void pdbg_disable_target(struct pdbg_target *target)
{
	struct dt_property *p;

	p = dt_find_property(target->dn, "status");
	if (p)
		/* We don't override hard-coded device tree
		 * status. This is needed to avoid disabling that
		 * backend. */
		return;

	dt_add_property_string(target->dn, "status", "disabled");
}

/* Searches up the tree and returns the first valid index found */
uint32_t pdbg_target_index(struct pdbg_target *target)
{
	struct dt_node *dn;

	for (dn = target->dn; dn && dn->target->index == -1; dn = dn->parent);

	if (!dn)
		return -1;
	else
		return dn->target->index;
}

/* Searched up the tree for the first target of the right class and returns its index */
uint32_t pdbg_parent_index(struct pdbg_target *target, char *class)
{
	struct pdbg_target *tmp;

	for (tmp = target; tmp && tmp->dn->parent; tmp = tmp->dn->parent->target) {
		if (!strcmp(class, pdbg_target_class_name(tmp)))
			return pdbg_target_index(tmp);
	}

	return -1;
}

char *pdbg_target_class_name(struct pdbg_target *target)
{
	return target->class;
}

char *pdbg_target_name(struct pdbg_target *target)
{
	return target->name;
}

void pdbg_set_target_property(struct pdbg_target *target, const char *name, const void *val, size_t size)
{
	struct dt_property *p;

	if ((p = dt_find_property(target->dn, name))) {
		if (size > p->len) {
			dt_resize_property(&p, size);
			p->len = size;
		}

		memcpy(p->prop, val, size);
	} else {
		dt_add_property(target->dn, name, val, size);
	}
}

void *pdbg_get_target_property(struct pdbg_target *target, const char *name, size_t *size)
{
	struct dt_property *p;

	p = dt_find_property(target->dn, name);
	if (p) {
		if (size)
			*size = p->len;
		return p->prop;
	} else if (size)
		*size = 0;

	return NULL;
}

uint64_t pdbg_get_address(struct pdbg_target *target, uint64_t *size)
{
	return dt_get_address(target->dn, 0, size);
}

/* Difference from below is that it doesn't search up the tree for the given
 * property. As nothing uses this yet we don't export it for use, but we may in
 * future */
static int pdbg_get_target_u64_property(struct pdbg_target *target, const char *name, uint64_t *val)
{
	struct dt_property *p;

	p = dt_find_property(target->dn, name);
	if (!p)
		return -1;

	*val = dt_get_number(p->prop, 2);
	return 0;
}

int pdbg_get_u64_property(struct pdbg_target *target, const char *name, uint64_t *val)
{
	struct dt_node *dn;

	for (dn = target->dn; dn; dn = dn->parent) {
		if (!pdbg_get_target_u64_property(dn->target, name, val))
			return 0;
	}

	return -1;
}
