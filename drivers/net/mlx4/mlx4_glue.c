/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018 6WIND S.A.
 * Copyright 2018 Mellanox
 */

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

#include "mlx4_glue.h"

static void
mlx4_glue_ack_async_event(struct ibv_async_event *event)
{
	ibv_ack_async_event(event);
}

static struct ibv_pd *
mlx4_glue_alloc_pd(struct ibv_context *context)
{
	return ibv_alloc_pd(context);
}

static int
mlx4_glue_close_device(struct ibv_context *context)
{
	return ibv_close_device(context);
}

static struct ibv_flow *
mlx4_glue_create_flow(struct ibv_qp *qp, struct ibv_flow_attr *flow)
{
	return ibv_create_flow(qp, flow);
}

static int
mlx4_glue_dealloc_pd(struct ibv_pd *pd)
{
	return ibv_dealloc_pd(pd);
}

static int
mlx4_glue_dereg_mr(struct ibv_mr *mr)
{
	return ibv_dereg_mr(mr);
}

static int
mlx4_glue_destroy_cq(struct ibv_cq *cq)
{
	return ibv_destroy_cq(cq);
}

static int
mlx4_glue_destroy_flow(struct ibv_flow *flow_id)
{
	return ibv_destroy_flow(flow_id);
}

static int
mlx4_glue_destroy_qp(struct ibv_qp *qp)
{
	return ibv_destroy_qp(qp);
}

static struct ibv_cq *
mlx4_glue_exp_create_cq(struct ibv_context *context, int cqe, void *cq_context,
			struct ibv_comp_channel *channel, int comp_vector,
			struct ibv_exp_cq_init_attr *attr)
{
	return ibv_exp_create_cq(context, cqe, cq_context, channel,
				 comp_vector, attr);
}

static struct ibv_qp *
mlx4_glue_exp_create_qp(struct ibv_context *context,
			struct ibv_exp_qp_init_attr *qp_init_attr)
{
	return ibv_exp_create_qp(context, qp_init_attr);
}

static struct ibv_exp_res_domain *
mlx4_glue_exp_create_res_domain(struct ibv_context *context,
				struct ibv_exp_res_domain_init_attr *attr)
{
	return ibv_exp_create_res_domain(context, attr);
}

static int
mlx4_glue_exp_destroy_res_domain(struct ibv_context *context,
				 struct ibv_exp_res_domain *res_dom,
				 struct ibv_exp_destroy_res_domain_attr *attr)
{
	return ibv_exp_destroy_res_domain(context, res_dom, attr);
}

static int
mlx4_glue_exp_modify_qp(struct ibv_qp *qp, struct ibv_exp_qp_attr *attr,
			uint64_t exp_attr_mask)
{
	return ibv_exp_modify_qp(qp, attr, exp_attr_mask);
}

static int
mlx4_glue_exp_query_device(struct ibv_context *context,
			   struct ibv_exp_device_attr *attr)
{
	return ibv_exp_query_device(context, attr);
}

static void *
mlx4_glue_exp_query_intf(struct ibv_context *context,
			 struct ibv_exp_query_intf_params *params,
			 enum ibv_exp_query_intf_status *status)
{
	return ibv_exp_query_intf(context, params, status);
}

static int
mlx4_glue_exp_release_intf(struct ibv_context *context, void *intf,
			   struct ibv_exp_release_intf_params *params)
{
	return ibv_exp_release_intf(context, intf, params);
}

static int
mlx4_glue_fork_init(void)
{
	return ibv_fork_init();
}

static void
mlx4_glue_free_device_list(struct ibv_device **list)
{
	ibv_free_device_list(list);
}

static int
mlx4_glue_get_async_event(struct ibv_context *context,
			  struct ibv_async_event *event)
{
	return ibv_get_async_event(context, event);
}

static struct ibv_device **
mlx4_glue_get_device_list(int *num_devices)
{
	return ibv_get_device_list(num_devices);
}

static const char *
mlx4_glue_get_device_name(struct ibv_device *device)
{
	return ibv_get_device_name(device);
}

static struct ibv_context *
mlx4_glue_open_device(struct ibv_device *device)
{
	return ibv_open_device(device);
}

static int
mlx4_glue_poll_cq(struct ibv_cq *cq, int num_entries, struct ibv_wc *wc)
{
	return ibv_poll_cq(cq, num_entries, wc);
}

static const char *
mlx4_glue_port_state_str(enum ibv_port_state port_state)
{
	return ibv_port_state_str(port_state);
}

static int
mlx4_glue_post_recv(struct ibv_qp *qp, struct ibv_recv_wr *wr,
		    struct ibv_recv_wr **bad_wr)
{
	return ibv_post_recv(qp, wr, bad_wr);
}

static int
mlx4_glue_query_device(struct ibv_context *context,
		       struct ibv_device_attr *device_attr)
{
	return ibv_query_device(context, device_attr);
}

static int
mlx4_glue_query_port(struct ibv_context *context, uint8_t port_num,
		     struct ibv_port_attr *port_attr)
{
	return ibv_query_port(context, port_num, port_attr);
}

static struct ibv_mr *
mlx4_glue_reg_mr(struct ibv_pd *pd, void *addr, size_t length, int access)
{
	return ibv_reg_mr(pd, addr, length, access);
}

static int
mlx4_glue_resize_cq(struct ibv_cq *cq, int cqe)
{
	return ibv_resize_cq(cq, cqe);
}

static const char *
mlx4_glue_wc_status_str(enum ibv_wc_status status)
{
	return ibv_wc_status_str(status);
}

const struct mlx4_glue *mlx4_glue = &(const struct mlx4_glue){
	.version = MLX4_GLUE_VERSION,
	.ack_async_event = mlx4_glue_ack_async_event,
	.alloc_pd = mlx4_glue_alloc_pd,
	.close_device = mlx4_glue_close_device,
	.create_flow = mlx4_glue_create_flow,
	.dealloc_pd = mlx4_glue_dealloc_pd,
	.dereg_mr = mlx4_glue_dereg_mr,
	.destroy_cq = mlx4_glue_destroy_cq,
	.destroy_flow = mlx4_glue_destroy_flow,
	.destroy_qp = mlx4_glue_destroy_qp,
	.exp_create_cq = mlx4_glue_exp_create_cq,
	.exp_create_qp = mlx4_glue_exp_create_qp,
	.exp_create_res_domain = mlx4_glue_exp_create_res_domain,
	.exp_destroy_res_domain = mlx4_glue_exp_destroy_res_domain,
	.exp_modify_qp = mlx4_glue_exp_modify_qp,
	.exp_query_device = mlx4_glue_exp_query_device,
	.exp_query_intf = mlx4_glue_exp_query_intf,
	.exp_release_intf = mlx4_glue_exp_release_intf,
	.fork_init = mlx4_glue_fork_init,
	.free_device_list = mlx4_glue_free_device_list,
	.get_async_event = mlx4_glue_get_async_event,
	.get_device_list = mlx4_glue_get_device_list,
	.get_device_name = mlx4_glue_get_device_name,
	.open_device = mlx4_glue_open_device,
	.poll_cq = mlx4_glue_poll_cq,
	.port_state_str = mlx4_glue_port_state_str,
	.post_recv = mlx4_glue_post_recv,
	.query_device = mlx4_glue_query_device,
	.query_port = mlx4_glue_query_port,
	.reg_mr = mlx4_glue_reg_mr,
	.resize_cq = mlx4_glue_resize_cq,
	.wc_status_str = mlx4_glue_wc_status_str,
};
