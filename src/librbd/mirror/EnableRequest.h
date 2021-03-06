// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef CEPH_LIBRBD_MIRROR_ENABLE_REQUEST_H
#define CEPH_LIBRBD_MIRROR_ENABLE_REQUEST_H

#include "include/buffer_fwd.h"
#include "include/rados/librados_fwd.hpp"
#include "include/rbd/librbd.hpp"
#include "cls/rbd/cls_rbd_types.h"
#include <map>
#include <string>

class Context;
class ContextWQ;

namespace librbd {

class ImageCtx;

namespace mirror {

template <typename ImageCtxT = ImageCtx>
class EnableRequest {
public:
  static EnableRequest *create(ImageCtxT *image_ctx, mirror_image_mode_t mode,
                               Context *on_finish) {
    return create(image_ctx->md_ctx, image_ctx->id, mode, "",
                  image_ctx->op_work_queue, on_finish);
  }
  static EnableRequest *create(librados::IoCtx &io_ctx,
                               const std::string &image_id,
                               mirror_image_mode_t mode,
                               const std::string &non_primary_global_image_id,
                               ContextWQ *op_work_queue, Context *on_finish) {
    return new EnableRequest(io_ctx, image_id, mode,
                             non_primary_global_image_id, op_work_queue,
                             on_finish);
  }

  void send();

private:
  /**
   * @verbatim
   *
   * <start>
   *    |
   *    v
   * GET_MIRROR_IMAGE * * * * * * *
   *    |                         * (on error)
   *    v                         *
   * GET_TAG_OWNER  * * * * * * * *
   *    |  (skip if not needed)   *
   *    v                         *
   * SET_MIRROR_IMAGE * * * * * * *
   *    |                         *
   *    v                         *
   * NOTIFY_MIRRORING_WATCHER * * *
   *    |                         *
   *    v                         *
   * <finish>   < * * * * * * * * *
   *
   * @endverbatim
   */

  EnableRequest(librados::IoCtx &io_ctx, const std::string &image_id,
                mirror_image_mode_t mode,
                const std::string &non_primary_global_image_id,
                ContextWQ *op_work_queue, Context *on_finish);

  librados::IoCtx &m_io_ctx;
  std::string m_image_id;
  cls::rbd::MirrorImageMode m_mode;
  std::string m_non_primary_global_image_id;
  ContextWQ *m_op_work_queue;
  Context *m_on_finish;

  CephContext *m_cct = nullptr;
  bool m_is_primary = false;
  bufferlist m_out_bl;
  cls::rbd::MirrorImage m_mirror_image;

  void send_get_mirror_image();
  Context *handle_get_mirror_image(int *result);

  void send_get_features();
  Context *handle_get_features(int *result);

  void send_get_tag_owner();
  Context *handle_get_tag_owner(int *result);

  void send_set_mirror_image();
  Context *handle_set_mirror_image(int *result);

  void send_notify_mirroring_watcher();
  Context *handle_notify_mirroring_watcher(int *result);
};

} // namespace mirror
} // namespace librbd

extern template class librbd::mirror::EnableRequest<librbd::ImageCtx>;

#endif // CEPH_LIBRBD_MIRROR_ENABLE_REQUEST_H
