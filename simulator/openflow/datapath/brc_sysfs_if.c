#include <linux/version.h>
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,18)

/*
 *	Sysfs attributes of bridge ports for OpenFlow
 *
 *  This has been shamelessly copied from the kernel sources.
 */

#include <linux/capability.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/if_bridge.h>
#include <linux/rtnetlink.h>
#include <linux/spinlock.h>

#include "brc_sysfs.h"
#include "datapath.h"

struct brport_attribute {
	struct attribute	attr;
	ssize_t (*show)(struct net_bridge_port *, char *);
	ssize_t (*store)(struct net_bridge_port *, unsigned long);
};

#define BRPORT_ATTR(_name,_mode,_show,_store)		        \
struct brport_attribute brport_attr_##_name = { 	        \
	.attr = {.name = __stringify(_name), 			\
		 .mode = _mode, 				\
		 .owner = THIS_MODULE, },			\
	.show	= _show,					\
	.store	= _store,					\
};

static ssize_t show_path_cost(struct net_bridge_port *p, char *buf)
{
#if 0
	return sprintf(buf, "%d\n", p->path_cost);
#else
	return sprintf(buf, "%d\n", 0);
#endif
}
static ssize_t store_path_cost(struct net_bridge_port *p, unsigned long v)
{
#if 0
	br_stp_set_path_cost(p, v);
#endif
	return 0;
}
static BRPORT_ATTR(path_cost, S_IRUGO | S_IWUSR,
		   show_path_cost, store_path_cost);

static ssize_t show_priority(struct net_bridge_port *p, char *buf)
{
#if 0
	return sprintf(buf, "%d\n", p->priority);
#else
	return sprintf(buf, "%d\n", 0);
#endif
}
static ssize_t store_priority(struct net_bridge_port *p, unsigned long v)
{
#if 0
	if (v >= (1<<(16-BR_PORT_BITS)))
		return -ERANGE;
	br_stp_set_port_priority(p, v);
#endif
	return 0;
}
static BRPORT_ATTR(priority, S_IRUGO | S_IWUSR,
			 show_priority, store_priority);

static ssize_t show_designated_root(struct net_bridge_port *p, char *buf)
{
#if 0
	return br_show_bridge_id(buf, &p->designated_root);
#else
	return sprintf(buf, "0000.010203040506\n");
#endif
}
static BRPORT_ATTR(designated_root, S_IRUGO, show_designated_root, NULL);

static ssize_t show_designated_bridge(struct net_bridge_port *p, char *buf)
{
#if 0
	return br_show_bridge_id(buf, &p->designated_bridge);
#else
	return sprintf(buf, "0000.060504030201\n");
#endif
}
static BRPORT_ATTR(designated_bridge, S_IRUGO, show_designated_bridge, NULL);

static ssize_t show_designated_port(struct net_bridge_port *p, char *buf)
{
#if 0
	return sprintf(buf, "%d\n", p->designated_port);
#else
	return sprintf(buf, "%d\n", 0);
#endif
}
static BRPORT_ATTR(designated_port, S_IRUGO, show_designated_port, NULL);

static ssize_t show_designated_cost(struct net_bridge_port *p, char *buf)
{
#if 0
	return sprintf(buf, "%d\n", p->designated_cost);
#else
	return sprintf(buf, "%d\n", 0);
#endif
}
static BRPORT_ATTR(designated_cost, S_IRUGO, show_designated_cost, NULL);

static ssize_t show_port_id(struct net_bridge_port *p, char *buf)
{
#if 0
	return sprintf(buf, "0x%x\n", p->port_id);
#else
	return sprintf(buf, "0x%x\n", 0);
#endif
}
static BRPORT_ATTR(port_id, S_IRUGO, show_port_id, NULL);

static ssize_t show_port_no(struct net_bridge_port *p, char *buf)
{
	return sprintf(buf, "0x%x\n", p->port_no);
}

static BRPORT_ATTR(port_no, S_IRUGO, show_port_no, NULL);

static ssize_t show_change_ack(struct net_bridge_port *p, char *buf)
{
#if 0
	return sprintf(buf, "%d\n", p->topology_change_ack);
#else
	return sprintf(buf, "%d\n", 0);
#endif
}
static BRPORT_ATTR(change_ack, S_IRUGO, show_change_ack, NULL);

static ssize_t show_config_pending(struct net_bridge_port *p, char *buf)
{
#if 0
	return sprintf(buf, "%d\n", p->config_pending);
#else
	return sprintf(buf, "%d\n", 0);
#endif
}
static BRPORT_ATTR(config_pending, S_IRUGO, show_config_pending, NULL);

static ssize_t show_port_state(struct net_bridge_port *p, char *buf)
{
#if 0
	return sprintf(buf, "%d\n", p->state);
#else
	return sprintf(buf, "%d\n", 0);
#endif
}
static BRPORT_ATTR(state, S_IRUGO, show_port_state, NULL);

static ssize_t show_message_age_timer(struct net_bridge_port *p,
					    char *buf)
{
#if 0
	return sprintf(buf, "%ld\n", br_timer_value(&p->message_age_timer));
#else
	return sprintf(buf, "%d\n", 0);
#endif
}
static BRPORT_ATTR(message_age_timer, S_IRUGO, show_message_age_timer, NULL);

static ssize_t show_forward_delay_timer(struct net_bridge_port *p,
					    char *buf)
{
#if 0
	return sprintf(buf, "%ld\n", br_timer_value(&p->forward_delay_timer));
#else
	return sprintf(buf, "%d\n", 0);
#endif
}
static BRPORT_ATTR(forward_delay_timer, S_IRUGO, show_forward_delay_timer, NULL);

static ssize_t show_hold_timer(struct net_bridge_port *p,
					    char *buf)
{
#if 0
	return sprintf(buf, "%ld\n", br_timer_value(&p->hold_timer));
#else
	return sprintf(buf, "%d\n", 0);
#endif
}
static BRPORT_ATTR(hold_timer, S_IRUGO, show_hold_timer, NULL);

static struct brport_attribute *brport_attrs[] = {
	&brport_attr_path_cost,
	&brport_attr_priority,
	&brport_attr_port_id,
	&brport_attr_port_no,
	&brport_attr_designated_root,
	&brport_attr_designated_bridge,
	&brport_attr_designated_port,
	&brport_attr_designated_cost,
	&brport_attr_state,
	&brport_attr_change_ack,
	&brport_attr_config_pending,
	&brport_attr_message_age_timer,
	&brport_attr_forward_delay_timer,
	&brport_attr_hold_timer,
	NULL
};

#define to_brport_attr(_at) container_of(_at, struct brport_attribute, attr)
#define to_brport(obj)	container_of(obj, struct net_bridge_port, kobj)

static ssize_t brport_show(struct kobject * kobj,
			   struct attribute * attr, char * buf)
{
	struct brport_attribute * brport_attr = to_brport_attr(attr);
	struct net_bridge_port * p = to_brport(kobj);

	return brport_attr->show(p, buf);
}

static ssize_t brport_store(struct kobject * kobj,
			    struct attribute * attr,
			    const char * buf, size_t count)
{
	struct net_bridge_port * p = to_brport(kobj);
#if 0
	struct brport_attribute * brport_attr = to_brport_attr(attr);
	char *endp;
	unsigned long val;
#endif
	ssize_t ret = -EINVAL;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

#if 0
	val = simple_strtoul(buf, &endp, 0);
	if (endp != buf) {
		rtnl_lock();
		if (p->dev && p->br && brport_attr->store) {
			spin_lock_bh(&p->br->lock);
			ret = brport_attr->store(p, val);
			spin_unlock_bh(&p->br->lock);
			if (ret == 0)
				ret = count;
		}
		rtnl_unlock();
	}
#else
	printk("%s: xxx writing port parms not supported yet!\n", 
			p->dp->netdev->name);
#endif
	return ret;
}

struct sysfs_ops brport_sysfs_ops = {
	.show = brport_show,
	.store = brport_store,
};

static void release_nbp(struct kobject *kobj)
{
	struct net_bridge_port *p
		= container_of(kobj, struct net_bridge_port, kobj);
	kfree(p);
}

struct kobj_type brport_ktype = {
	.sysfs_ops = &brport_sysfs_ops,
	.release = release_nbp
};

/*
 * Add sysfs entries to ethernet device added to a bridge.
 * Creates a brport subdirectory with bridge attributes.
 * Puts symlink in bridge's brport subdirectory
 */
int brc_sysfs_add_if(struct net_bridge_port *p)
{
	struct datapath *dp = p->dp;
	struct brport_attribute **a;
	int err;

	kobject_init(&p->kobj);
	kobject_set_name(&p->kobj, SYSFS_BRIDGE_PORT_ATTR);
	p->kobj.ktype = &brport_ktype;
	p->kobj.kset = NULL;
	p->kobj.parent = &(p->dev->class_dev.kobj);

	err = kobject_add(&p->kobj);
	if (err)
		goto err_put;

	err = sysfs_create_link(&p->kobj, &dp->netdev->class_dev.kobj, 
				SYSFS_BRIDGE_PORT_LINK);
	if (err)
		goto err_del;

	for (a = brport_attrs; *a; ++a) {
		err = sysfs_create_file(&p->kobj, &((*a)->attr));
		if (err)
			goto err_del;
	}

	err = sysfs_create_link(&dp->ifobj, &p->kobj, p->dev->name);
	if (err)
		goto err_del;

	kobject_uevent(&p->kobj, KOBJ_ADD);

	return err;

err_del:
	kobject_del(&p->kobj);
err_put:
	kobject_put(&p->kobj);
	return err;
}

int brc_sysfs_del_if(struct net_bridge_port *p)
{
	struct net_device *dev = p->dev;

	kobject_uevent(&p->kobj, KOBJ_REMOVE);
	kobject_del(&p->kobj);

	dev_put(dev);

	kobject_put(&p->kobj);

	return 0;
}
#endif /* Only support 2.6.18 */
