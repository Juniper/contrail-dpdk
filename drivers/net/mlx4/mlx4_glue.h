/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018 6WIND S.A.
 * Copyright 2018 Mellanox
 */

#ifndef MLX4_GLUE_H_
#define MLX4_GLUE_H_

#include <stddef.h>
#include <stdint.h>

/* Verbs headers do not support -pedantic. */
#ifdef PEDANTIC
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <infiniband/verbs.h>
#include <infiniband/verbs_exp.h>
#ifdef PEDANTIC
#pragma GCC diagnostic error "-Wpedantic"
#endif

#ifndef MLX4_GLUE_VERSION
#define MLX4_GLUE_VERSION ""
#endif

/* LIB_GLUE_VERSION must be updated every time this structure is modified. */
struct mlx4_glue {
	const char *version;
	void (*ack_async_event)(struct ibv_async_event *event);
	struct ibv_pd *(*alloc_pd)(struct ibv_context *context);
	int (*close_device)(struct ibv_context *context);
	struct ibv_flow *(*create_flow)(struct ibv_qp *qp,
					struct ibv_flow_attr *flow);
	int (*dealloc_pd)(struct ibv_pd *pd);
	int (*dereg_mr)(struct ibv_mr *mr);
	int (*destroy_cq)(struct ibv_cq *cq);
	int (*destroy_flow)(struct ibv_flow *flow_id);
	int (*destroy_qp)(struct ibv_qp *qp);
	struct ibv_cq *(*exp_create_cq)
		(struct ibv_context *context, int cqe, void *cq_context,
		 struct ibv_comp_channel *channel, int comp_vector,
		 struct ibv_exp_cq_init_attr *attr);
	struct ibv_qp *(*exp_create_qp)
		(struct ibv_context *context,
		 struct ibv_exp_qp_init_attr *qp_init_attr);
	struct ibv_exp_res_domain *(*exp_create_res_domain)
		(struct ibv_context *context,
		 struct ibv_exp_res_domain_init_attr *attr);
	int (*exp_destroy_res_domain)
		(struct ibv_context *context,
		 struct ibv_exp_res_domain *res_dom,
		 struct ibv_exp_destroy_res_domain_attr *attr);
	int (*exp_modify_qp)(struct ibv_qp *qp, struct ibv_exp_qp_attr *attr,
			     uint64_t exp_attr_mask);
	int (*exp_query_device)(struct ibv_context *context,
				struct ibv_exp_device_attr *attr);
	void *(*exp_query_intf)(struct ibv_context *context,
				struct ibv_exp_query_intf_params *params,
				enum ibv_exp_query_intf_status *status);
	int (*exp_release_intf)(struct ibv_context *context, void *intf,
				struct ibv_exp_release_intf_params *params);
	int (*fork_init)(void);
	void (*free_device_list)(struct ibv_device **list);
	int (*get_async_event)(struct ibv_context *context,
			       struct ibv_async_event *event);
	struct ibv_device **(*get_device_list)(int *num_devices);
	const char *(*get_device_name)(struct ibv_device *device);
	struct ibv_context *(*open_device)(struct ibv_device *device);
	int (*poll_cq)(struct ibv_cq *cq, int num_entries, struct ibv_wc *wc);
	const char *(*port_state_str)(enum ibv_port_state port_state);
	int (*post_recv)(struct ibv_qp *qp, struct ibv_recv_wr *wr,
			 struct ibv_recv_wr **bad_wr);
	int (*query_device)(struct ibv_context *context,
			    struct ibv_device_attr *device_attr);
	int (*query_port)(struct ibv_context *context, uint8_t port_num,
			  struct ibv_port_attr *port_attr);
	struct ibv_mr *(*reg_mr)(struct ibv_pd *pd, void *addr, size_t length,
				 int access);
	int (*resize_cq)(struct ibv_cq *cq, int cqe);
	const char *(*wc_status_str)(enum ibv_wc_status status);
};

const struct mlx4_glue *mlx4_glue;

#endif /* MLX4_GLUE_H_ */
