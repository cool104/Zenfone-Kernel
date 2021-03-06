/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *   Jesse Barnes <jbarnes@virtuousgeek.org>
 *
 * New plane/sprite handling.
 *
 * The older chips had a separate interface for programming plane related
 * registers; newer ones are much simpler and we can use the new DRM plane
 * support.
 */
#include "drmP.h"
#include "drm_crtc.h"
#include "drm_fourcc.h"
#include "intel_drv.h"
#include "i915_drm.h"
#include "i915_drv.h"

void
__alpha_setting_noncursor(u32 pixformat, int plane, u32 *dspcntr, int alpha)
{
	/* For readability, can split to individual cases */
	/* 5 no alphas, 6-9 common, a-d reserved for sprite, e-f common */
	switch (pixformat) {
	case DISPPLANE_RGBX888:
	case DISPPLANE_RGBA888:
		if (alpha)
			*dspcntr |= DISPPLANE_RGBA888;
		else
			*dspcntr |= DISPPLANE_RGBX888;
		break;
	case DISPPLANE_BGRX888:
	case DISPPLANE_BGRA888:
		if (alpha)
			*dspcntr |= DISPPLANE_BGRA888;
		else
			*dspcntr |= DISPPLANE_BGRX888;
		break;
	case DISPPLANE_RGBX101010:
	case DISPPLANE_RGBA101010:
		if (alpha)
			*dspcntr |= DISPPLANE_RGBA101010;
		else
			*dspcntr |= DISPPLANE_RGBX101010;
		break;
	case DISPPLANE_BGRX101010:
	case DISPPLANE_BGRA101010:
		if (alpha)
			*dspcntr |= DISPPLANE_BGRA101010;
		else
			*dspcntr |= DISPPLANE_BGRX101010;
		break;
	case DISPPLANE_RGBX161616:
	case DISPPLANE_RGBA161616:
		if ((plane == PLANEA) || (plane == PLANEB)) {
			if (alpha)
				*dspcntr |= DISPPLANE_RGBA161616;
			else
				*dspcntr |= DISPPLANE_RGBX161616;
		}
		break;
	default:
		DRM_ERROR("Unknown pixel format 0x%08x\n", pixformat);
		break;
	}
}

void
__alpha_setting_cursor(u32 pixformat, int plane, u32 *dspcntr, int alpha)
{
	/* For readability, can split to individual cases */
	switch (pixformat) {
	case CURSOR_MODE_128_32B_AX:
	case CURSOR_MODE_128_ARGB_AX:
		if (alpha)
			*dspcntr |= CURSOR_MODE_128_ARGB_AX;
		else
			*dspcntr |= CURSOR_MODE_128_32B_AX;
		break;

	case CURSOR_MODE_256_ARGB_AX:
	case CURSOR_MODE_256_32B_AX:
		if (alpha)
			*dspcntr |= CURSOR_MODE_256_ARGB_AX;
		else
			*dspcntr |= CURSOR_MODE_256_32B_AX;
		break;

	case CURSOR_MODE_64_ARGB_AX:
	case CURSOR_MODE_64_32B_AX:
		if (alpha)
			*dspcntr |= CURSOR_MODE_64_ARGB_AX;
		else
			*dspcntr |= CURSOR_MODE_64_32B_AX;
		break;
	default:
		DRM_ERROR("Unknown pixel format:Cursor 0x%08x\n", pixformat);
		break;
	}
}
/*
 * enable/disable alpha for planes
 */
int
i915_set_plane_alpha(struct drm_device *dev, void *data, struct drm_file *file)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_set_plane_alpha *alphadata = data;
	int plane = alphadata->plane;
	bool alpha = alphadata->alpha;
	bool IsCursor = false;
	u32 dspcntr;
	u32 reg = 0;
	u32 pixformat;
	u32 mask = DISPPLANE_PIXFORMAT_MASK;

	DRM_DEBUG_DRIVER("In i915_set_plane_alpha\n");

	switch (plane) {
	case PLANEA:
		reg = DSPCNTR(0);
		break;
	case PLANEB:
		reg = DSPCNTR(1);
		break;
	case SPRITEA:
		reg = SPCNTR(0, 0);
		break;
	case SPRITEB:
		reg = SPCNTR(0, 1);
		break;
	case SPRITEC:
		reg = SPCNTR(1, 0);
		break;
	case SPRITED:
		reg = SPCNTR(1, 1);
		break;
	case CURSORA:
		reg = CURCNTR(0);
		mask = CURSOR_MODE;
		IsCursor = true;
		break;
	case CURSORB:
		reg = CURCNTR(1);
		mask = CURSOR_MODE;
		IsCursor = true;
		break;
	default:
		DRM_ERROR("No plane selected properly\n");
		break;
	}

	dspcntr = I915_READ(reg);
	DRM_DEBUG_DRIVER("dspcntr = %x\n", dspcntr);

	pixformat = dspcntr & mask;
	dspcntr &= ~mask;
	DRM_DEBUG_DRIVER("pixformat = %x, alpha = %x\n", pixformat, alpha);

	if (pixformat) {
		if (!IsCursor)
			__alpha_setting_noncursor(pixformat, plane,
						&dspcntr, alpha);
		else
			__alpha_setting_cursor(pixformat, plane,
						&dspcntr, alpha);

		DRM_DEBUG_DRIVER("Reg should be written with = %x\n", dspcntr);

		if (pixformat != (dspcntr & mask)) {
			I915_WRITE(reg, dspcntr);
			DRM_DEBUG_DRIVER("Reg written with = %x\n", dspcntr);
		}
	} else
		DRM_DEBUG_DRIVER("Plane might not be enabled/configured!\n");

	return 0;
}

/*
 * enable/disable primary plane alpha channel based on the z-order
 */
void
i915_set_primary_alpha(struct drm_i915_private *dev_priv, int zorder, int plane)
{
	u32 dspcntr;
	u32 reg;
	u32 pixformat;
	bool alpha = false;

	if (zorder != P1S1S2C1 && zorder != P1S2S1C1)
		alpha = true;
	else
		alpha = false;

	reg = DSPCNTR(plane);
	dspcntr = I915_READ(reg);

	if (!(dspcntr & DISPLAY_PLANE_ENABLE))
		return ;

	pixformat = dspcntr & DISPPLANE_PIXFORMAT_MASK;

	dspcntr &= ~DISPPLANE_PIXFORMAT_MASK;

	DRM_DEBUG_DRIVER("pixformat = %x, alpha = %d", pixformat, alpha);

	switch (pixformat) {
	case DISPPLANE_BGRX888:
	case DISPPLANE_BGRA888:
		if (alpha)
			dspcntr |= DISPPLANE_BGRA888;
		else
			dspcntr |= DISPPLANE_BGRX888;
		break;
	case DISPPLANE_RGBX888:
	case DISPPLANE_RGBA888:
		if (alpha)
			dspcntr |= DISPPLANE_RGBA888;
		else
			dspcntr |= DISPPLANE_RGBX888;
		break;
	case DISPPLANE_BGRX101010:
	case DISPPLANE_BGRA101010:
		if (alpha)
			dspcntr |= DISPPLANE_BGRA101010;
		else
			dspcntr |= DISPPLANE_BGRX101010;
		break;
	case DISPPLANE_RGBX101010:
	case DISPPLANE_RGBA101010:
		if (alpha)
			dspcntr |= DISPPLANE_RGBA101010;
		else
			dspcntr |= DISPPLANE_RGBX101010;
		break;
	case DISPPLANE_BGRX565:
		dspcntr |= DISPPLANE_BGRX565;
		break;
	case DISPPLANE_8BPP:
		dspcntr |= DISPPLANE_8BPP;
		break;
	default:
		DRM_ERROR("Unknown pixel format 0x%08x\n", pixformat);
		break;
	}

	if (pixformat != (dspcntr & DISPPLANE_PIXFORMAT_MASK)) {
		I915_WRITE(reg, dspcntr);
		DRM_DEBUG_DRIVER("dspcntr = %x", dspcntr);
	}
}

/*
 * enable/disable sprite alpha channel based on the z-order
 */
void i915_set_sprite_alpha(struct drm_i915_private *dev_priv, int zorder,
				int pipe, int plane)
{
	u32 spcntr;
	u32 pixformat;
	bool alpha = false;

	if (zorder != S1P1S2C1 && zorder != S1S2P1C1 && plane == 0)
		alpha = true;
	else if (zorder != S2P1S1C1 && zorder != S2S1P1C1 && plane == 1)
		alpha = true;
	else
		alpha = false;

	spcntr = I915_READ(SPCNTR(pipe, plane));
	if (!(spcntr & DISPLAY_PLANE_ENABLE))
		return ;

	pixformat = spcntr & SP_PIXFORMAT_MASK;
	spcntr &= ~SP_PIXFORMAT_MASK;

	DRM_DEBUG_DRIVER("sprite pixformat = %x plane = %d", pixformat, plane);

	switch (pixformat) {
	case SP_FORMAT_BGRA8888:
	case SP_FORMAT_BGRX8888:
		if (alpha)
			spcntr |= SP_FORMAT_BGRA8888;
		else
			spcntr |= SP_FORMAT_BGRX8888;
		break;
	case SP_FORMAT_RGBA8888:
	case SP_FORMAT_RGBX8888:
		if (alpha)
			spcntr |= SP_FORMAT_RGBA8888;
		else
			spcntr |= SP_FORMAT_RGBX8888;
		break;
	case SP_FORMAT_RGBA1010102:
	case SP_FORMAT_RGBX1010102:
		if (alpha)
			spcntr |= SP_FORMAT_RGBA1010102;
		else
			spcntr |= SP_FORMAT_RGBX1010102;
		break;
	case SP_FORMAT_YUV422:
		spcntr |= SP_FORMAT_YUV422;
		break;
	case SP_FORMAT_BGR565:
		spcntr |= SP_FORMAT_BGR565;
		break;
	default:
		DRM_ERROR("Unknown pixel format 0x%08x\n", pixformat);
		break;
	}

	if (pixformat != (spcntr & SP_PIXFORMAT_MASK)) {
		I915_WRITE(SPCNTR(pipe, plane), spcntr);
		DRM_DEBUG_DRIVER("spcntr = %x ", spcntr);
	}
}

int i915_set_plane_zorder(struct drm_device *dev, void *data,
			  struct drm_file *file)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 val;
	struct drm_i915_set_plane_zorder *zorder = data;
	u32 order = zorder->order;
	int s1_zorder, s1_bottom, s2_zorder, s2_bottom;
	int pipe = (order >> 31) & 0x1;
	int z_order = order & 0x000F;

	s1_zorder = (order >> 3) & 0x1;
	s1_bottom = (order >> 2) & 0x1;
	s2_zorder = (order >> 1) & 0x1;
	s2_bottom = (order >> 0) & 0x1;

	/* Clear the older Z-order */
	val = I915_READ(SPCNTR(pipe, 0));
	val &= ~(SPRITE_FORCE_BOTTOM | SPRITE_ZORDER_ENABLE);
	I915_WRITE(SPCNTR(pipe, 0), val);

	val = I915_READ(SPCNTR(pipe, 1));
	val &= ~(SPRITE_FORCE_BOTTOM | SPRITE_ZORDER_ENABLE);
	I915_WRITE(SPCNTR(pipe, 1), val);

	/* Program new Z-order */
	val = I915_READ(SPCNTR(pipe, 0));
	if (s1_zorder)
		val |= SPRITE_ZORDER_ENABLE;
	if (s1_bottom)
		val |= SPRITE_FORCE_BOTTOM;
	I915_WRITE(SPCNTR(pipe, 0), val);

	val = I915_READ(SPCNTR(pipe, 1));
	if (s2_zorder)
		val |= SPRITE_ZORDER_ENABLE;
	if (s2_bottom)
		val |= SPRITE_FORCE_BOTTOM;
	I915_WRITE(SPCNTR(pipe, 1), val);

	i915_set_primary_alpha(dev_priv, z_order, pipe);

	i915_set_sprite_alpha(dev_priv, z_order, pipe, 0);
	i915_set_sprite_alpha(dev_priv, z_order, pipe, 1);

	return 0;
}

static void
vlv_update_plane(struct drm_plane *dplane, struct drm_framebuffer *fb,
		 struct drm_i915_gem_object *obj, int crtc_x, int crtc_y,
		 unsigned int crtc_w, unsigned int crtc_h,
		 uint32_t x, uint32_t y,
		 uint32_t src_w, uint32_t src_h)
{
	struct drm_device *dev = dplane->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_plane *intel_plane = to_intel_plane(dplane);
	int pipe = intel_plane->pipe;
	int plane = intel_plane->plane;
	u32 sprctl;
	bool rotate = false;
	unsigned long sprsurf_offset, linear_offset;
	int pixel_size = drm_format_plane_cpp(fb->pixel_format, 0);

	sprctl = I915_READ(SPCNTR(pipe, plane));
	/* Mask out pixel format bits in case we change it */
	sprctl &= ~SP_PIXFORMAT_MASK;
	sprctl &= ~SP_YUV_BYTE_ORDER_MASK;
	sprctl &= ~SP_TILED;

	switch (fb->pixel_format) {
	case DRM_FORMAT_YUYV:
		sprctl |= SP_FORMAT_YUV422 | SP_YUV_ORDER_YUYV;
		break;
	case DRM_FORMAT_YVYU:
		sprctl |= SP_FORMAT_YUV422 | SP_YUV_ORDER_YVYU;
		break;
	case DRM_FORMAT_UYVY:
		sprctl |= SP_FORMAT_YUV422 | SP_YUV_ORDER_UYVY;
		break;
	case DRM_FORMAT_VYUY:
		sprctl |= SP_FORMAT_YUV422 | SP_YUV_ORDER_VYUY;
		break;
	case DRM_FORMAT_RGB565:
		sprctl |= SP_FORMAT_BGR565;
		break;
	case DRM_FORMAT_XRGB8888:
		sprctl |= SP_FORMAT_BGRX8888;
		break;
	case DRM_FORMAT_ARGB8888:
		sprctl |= SP_FORMAT_BGRA8888;
		break;
	case DRM_FORMAT_XBGR2101010:
		sprctl |= SP_FORMAT_RGBX1010102;
		break;
	case DRM_FORMAT_ABGR2101010:
		sprctl |= SP_FORMAT_RGBA1010102;
		break;
	case DRM_FORMAT_XBGR8888:
		sprctl |= SP_FORMAT_RGBX8888;
		break;
	case DRM_FORMAT_ABGR8888:
		sprctl |= SP_FORMAT_RGBA8888;
		break;
	default:
		/*
		 * If we get here one of the upper layers failed to filter
		 * out the unsupported plane formats
		 */
		BUG();
		break;
	}

	if (obj->tiling_mode != I915_TILING_NONE)
		sprctl |= SP_TILED;

	sprctl |= SP_ENABLE;

	if (sprctl & DISPPLANE_180_ROTATION_ENABLE)
		rotate = true;

	/* Sizes are 0 based */
	src_w--;
	src_h--;
	crtc_w--;
	crtc_h--;

	/*
	 * Disable Max Fifo configuration when sprite plane is enabled.
	 * Do not disable if Max Fifo is already disabled.
	 */

	if (I915_READ(FW_BLC_SELF_VLV) & FW_CSPWRDWNEN) {
		I915_WRITE(FW_BLC_SELF_VLV,
			I915_READ(FW_BLC_SELF_VLV) & ~FW_CSPWRDWNEN);
	}

	intel_update_sprite_watermarks(dev, pipe, crtc_w, pixel_size);
	I915_WRITE(SPSTRIDE(pipe, plane), fb->pitches[0]);
	if (rotate)
		I915_WRITE(SPPOS(pipe, plane), ((rot_mode.vdisplay -
			(crtc_y + crtc_h + 1)) << 16) |
				(rot_mode.hdisplay - (crtc_x + crtc_w + 1)));
	else
		I915_WRITE(SPPOS(pipe, plane), (crtc_y << 16) | crtc_x);

	linear_offset = y * fb->pitches[0] + x * pixel_size;
	sprsurf_offset = intel_gen4_compute_page_offset(&x, &y,
							obj->tiling_mode,
							pixel_size,
							fb->pitches[0]);
	linear_offset -= sprsurf_offset;

	if (obj->tiling_mode != I915_TILING_NONE) {
		if (rotate) {
			I915_WRITE(SPTILEOFF(pipe, plane),
				(((crtc_h + 1) << 16) | (crtc_w + 1)));
		} else
			I915_WRITE(SPTILEOFF(pipe, plane), (y << 16) | x);
	} else {
		if (rotate) {
			I915_WRITE(SPLINOFF(pipe, plane),
				(((crtc_h + 1) * (crtc_w + 1) *
				pixel_size)) - pixel_size);
		} else
			I915_WRITE(SPLINOFF(pipe, plane), linear_offset);
	}
	I915_WRITE(SPSIZE(pipe, plane), (crtc_h << 16) | crtc_w);
	if (rotate)
		sprctl |= DISPPLANE_180_ROTATION_ENABLE;

	I915_WRITE(SPCNTR(pipe, plane), sprctl);
	I915_MODIFY_DISPBASE(SPSURF(pipe, plane), obj->gtt_offset +
			     sprsurf_offset);
	POSTING_READ(SPSURF(pipe, plane));
}

static void
vlv_disable_plane(struct drm_plane *dplane)
{
	struct drm_device *dev = dplane->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_plane *intel_plane = to_intel_plane(dplane);
	int pipe = intel_plane->pipe;
	int plane = intel_plane->plane;

	I915_WRITE(SPCNTR(pipe, plane), I915_READ(SPCNTR(pipe, plane)) &
		   ~SP_ENABLE);

	/*
	 * Check if Max Fifo configuration is required when sprite
	 * is disabled.
	 */

	if (is_maxfifo_needed(dev_priv))
		I915_WRITE(FW_BLC_SELF_VLV, FW_CSPWRDWNEN);

	/* Activate double buffered register update */
	I915_MODIFY_DISPBASE(SPSURF(pipe, plane), 0);
	POSTING_READ(SPSURF(pipe, plane));
}

static int
vlv_update_colorkey(struct drm_plane *dplane,
		    struct drm_intel_sprite_colorkey *key)
{
	struct drm_device *dev = dplane->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_plane *intel_plane = to_intel_plane(dplane);
	int pipe = intel_plane->pipe;
	int plane = intel_plane->plane;
	u32 sprctl;

	if (!(key->flags &
	      (I915_SET_COLORKEY_SOURCE | I915_SET_COLORKEY_ALPHA)))
		return -EINVAL;

	I915_WRITE(SPKEYMINVAL(pipe, plane), key->min_value);
	I915_WRITE(SPKEYMAXVAL(pipe, plane), key->max_value);
	I915_WRITE(SPKEYMSK(pipe, plane), key->channel_mask);

	sprctl = I915_READ(SPCNTR(pipe, plane));
	sprctl &= ~SP_SOURCE_KEY;

	if (!(key->flags & I915_SET_COLORKEY_ALPHA))
		I915_WRITE(SPCONSTALPHA(pipe, plane), 0);

	if (key->flags & I915_SET_COLORKEY_SOURCE)
		sprctl |= SP_SOURCE_KEY;
	else if (key->flags & I915_SET_COLORKEY_ALPHA) {
		I915_WRITE(SPCONSTALPHA(pipe, plane),
			   SP_ALPHA_EN | key->channel_mask);
	}
	I915_WRITE(SPCNTR(pipe, plane), sprctl);

	POSTING_READ(SPKEYMSK(pipe, plane));

	return 0;
}

static void
vlv_get_colorkey(struct drm_plane *dplane,
		 struct drm_intel_sprite_colorkey *key)
{
	struct drm_device *dev = dplane->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_plane *intel_plane = to_intel_plane(dplane);
	int pipe = intel_plane->pipe;
	int plane = intel_plane->plane;
	u32 sprctl;

	key->min_value = I915_READ(SPKEYMINVAL(pipe, plane));
	key->max_value = I915_READ(SPKEYMAXVAL(pipe, plane));
	key->channel_mask = I915_READ(SPKEYMSK(pipe, plane));

	sprctl = I915_READ(SPCNTR(pipe, plane));
	if (sprctl & SP_SOURCE_KEY)
		key->flags = I915_SET_COLORKEY_SOURCE;
	else
		key->flags = I915_SET_COLORKEY_NONE;
}

static void
ivb_update_plane(struct drm_plane *plane, struct drm_framebuffer *fb,
		 struct drm_i915_gem_object *obj, int crtc_x, int crtc_y,
		 unsigned int crtc_w, unsigned int crtc_h,
		 uint32_t x, uint32_t y,
		 uint32_t src_w, uint32_t src_h)
{
	struct drm_device *dev = plane->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_plane *intel_plane = to_intel_plane(plane);
	int pipe = intel_plane->pipe;
	u32 sprctl, sprscale = 0;
	int pixel_size;
	unsigned long sprsurf_offset, linear_offset;

	sprctl = I915_READ(SPRCTL(pipe));

	/* Mask out pixel format bits in case we change it */
	sprctl &= ~SPRITE_PIXFORMAT_MASK;
	sprctl &= ~SPRITE_RGB_ORDER_RGBX;
	sprctl &= ~SPRITE_YUV_BYTE_ORDER_MASK;
	sprctl &= ~SPRITE_TILED;

	switch (fb->pixel_format) {
	case DRM_FORMAT_XBGR8888:
		sprctl |= SPRITE_FORMAT_RGBX888;
		pixel_size = 4;
		break;
	case DRM_FORMAT_XRGB8888:
		sprctl |= SPRITE_FORMAT_RGBX888 | SPRITE_RGB_ORDER_RGBX;
		pixel_size = 4;
		break;
	case DRM_FORMAT_YUYV:
		sprctl |= SPRITE_FORMAT_YUV422 | SPRITE_YUV_ORDER_YUYV;
		pixel_size = 2;
		break;
	case DRM_FORMAT_YVYU:
		sprctl |= SPRITE_FORMAT_YUV422 | SPRITE_YUV_ORDER_YVYU;
		pixel_size = 2;
		break;
	case DRM_FORMAT_UYVY:
		sprctl |= SPRITE_FORMAT_YUV422 | SPRITE_YUV_ORDER_UYVY;
		pixel_size = 2;
		break;
	case DRM_FORMAT_VYUY:
		sprctl |= SPRITE_FORMAT_YUV422 | SPRITE_YUV_ORDER_VYUY;
		pixel_size = 2;
		break;
	default:
		DRM_DEBUG_DRIVER("bad pixel format, assuming RGBX888\n");
		sprctl |= SPRITE_FORMAT_RGBX888;
		pixel_size = 4;
		break;
	}

	if (obj->tiling_mode != I915_TILING_NONE)
		sprctl |= SPRITE_TILED;

	/* must disable */
	sprctl |= SPRITE_TRICKLE_FEED_DISABLE;
	sprctl |= SPRITE_ENABLE;

	/* Sizes are 0 based */
	src_w--;
	src_h--;
	crtc_w--;
	crtc_h--;

	intel_update_sprite_watermarks(dev, pipe, crtc_w, pixel_size);

	/*
	 * IVB workaround: must disable low power watermarks for at least
	 * one frame before enabling scaling.  LP watermarks can be re-enabled
	 * when scaling is disabled.
	 */
	if (crtc_w != src_w || crtc_h != src_h) {
		if (!dev_priv->sprite_scaling_enabled) {
			dev_priv->sprite_scaling_enabled = true;
			intel_update_watermarks(dev);
			intel_wait_for_vblank(dev, pipe);
		}
		sprscale = SPRITE_SCALE_ENABLE | (src_w << 16) | src_h;
	} else {
		if (dev_priv->sprite_scaling_enabled) {
			dev_priv->sprite_scaling_enabled = false;
			/* potentially re-enable LP watermarks */
			intel_update_watermarks(dev);
		}
	}

	I915_WRITE(SPRSTRIDE(pipe), fb->pitches[0]);
	I915_WRITE(SPRPOS(pipe), (crtc_y << 16) | crtc_x);

	linear_offset = y * fb->pitches[0] + x * (fb->bits_per_pixel / 8);
	sprsurf_offset =
		intel_gen4_compute_page_offset(&x, &y, obj->tiling_mode,
					       pixel_size, fb->pitches[0]);
	linear_offset -= sprsurf_offset;

	if (obj->tiling_mode != I915_TILING_NONE)
		I915_WRITE(SPRTILEOFF(pipe), (y << 16) | x);
	else
		I915_WRITE(SPRLINOFF(pipe), linear_offset);

	I915_WRITE(SPRSIZE(pipe), (crtc_h << 16) | crtc_w);
	I915_WRITE(SPRSCALE(pipe), sprscale);
	I915_WRITE(SPRCTL(pipe), sprctl);
	I915_MODIFY_DISPBASE(SPRSURF(pipe), obj->gtt_offset + sprsurf_offset);
	POSTING_READ(SPRSURF(pipe));
}

static void
ivb_disable_plane(struct drm_plane *plane)
{
	struct drm_device *dev = plane->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_plane *intel_plane = to_intel_plane(plane);
	int pipe = intel_plane->pipe;

	I915_WRITE(SPRCTL(pipe), I915_READ(SPRCTL(pipe)) & ~SPRITE_ENABLE);
	/* Can't leave the scaler enabled... */
	I915_WRITE(SPRSCALE(pipe), 0);
	/* Activate double buffered register update */
	I915_MODIFY_DISPBASE(SPRSURF(pipe), 0);
	POSTING_READ(SPRSURF(pipe));

	dev_priv->sprite_scaling_enabled = false;
	intel_update_watermarks(dev);
}

static int
ivb_update_colorkey(struct drm_plane *plane,
		    struct drm_intel_sprite_colorkey *key)
{
	struct drm_device *dev = plane->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_plane *intel_plane;
	u32 sprctl;
	int ret = 0;

	intel_plane = to_intel_plane(plane);

	I915_WRITE(SPRKEYVAL(intel_plane->pipe), key->min_value);
	I915_WRITE(SPRKEYMAX(intel_plane->pipe), key->max_value);
	I915_WRITE(SPRKEYMSK(intel_plane->pipe), key->channel_mask);

	sprctl = I915_READ(SPRCTL(intel_plane->pipe));
	sprctl &= ~(SPRITE_SOURCE_KEY | SPRITE_DEST_KEY);
	if (key->flags & I915_SET_COLORKEY_DESTINATION)
		sprctl |= SPRITE_DEST_KEY;
	else if (key->flags & I915_SET_COLORKEY_SOURCE)
		sprctl |= SPRITE_SOURCE_KEY;
	I915_WRITE(SPRCTL(intel_plane->pipe), sprctl);

	POSTING_READ(SPRKEYMSK(intel_plane->pipe));

	return ret;
}

static void
ivb_get_colorkey(struct drm_plane *plane, struct drm_intel_sprite_colorkey *key)
{
	struct drm_device *dev = plane->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_plane *intel_plane;
	u32 sprctl;

	intel_plane = to_intel_plane(plane);

	key->min_value = I915_READ(SPRKEYVAL(intel_plane->pipe));
	key->max_value = I915_READ(SPRKEYMAX(intel_plane->pipe));
	key->channel_mask = I915_READ(SPRKEYMSK(intel_plane->pipe));
	key->flags = 0;

	sprctl = I915_READ(SPRCTL(intel_plane->pipe));

	if (sprctl & SPRITE_DEST_KEY)
		key->flags = I915_SET_COLORKEY_DESTINATION;
	else if (sprctl & SPRITE_SOURCE_KEY)
		key->flags = I915_SET_COLORKEY_SOURCE;
	else
		key->flags = I915_SET_COLORKEY_NONE;
}

static void
ilk_update_plane(struct drm_plane *plane, struct drm_framebuffer *fb,
		 struct drm_i915_gem_object *obj, int crtc_x, int crtc_y,
		 unsigned int crtc_w, unsigned int crtc_h,
		 uint32_t x, uint32_t y,
		 uint32_t src_w, uint32_t src_h)
{
	struct drm_device *dev = plane->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_plane *intel_plane = to_intel_plane(plane);
	int pipe = intel_plane->pipe, pixel_size;
	unsigned long dvssurf_offset, linear_offset;
	u32 dvscntr, dvsscale;

	dvscntr = I915_READ(DVSCNTR(pipe));

	/* Mask out pixel format bits in case we change it */
	dvscntr &= ~DVS_PIXFORMAT_MASK;
	dvscntr &= ~DVS_RGB_ORDER_XBGR;
	dvscntr &= ~DVS_YUV_BYTE_ORDER_MASK;
	dvscntr &= ~DVS_TILED;

	switch (fb->pixel_format) {
	case DRM_FORMAT_XBGR8888:
		dvscntr |= DVS_FORMAT_RGBX888 | DVS_RGB_ORDER_XBGR;
		pixel_size = 4;
		break;
	case DRM_FORMAT_XRGB8888:
		dvscntr |= DVS_FORMAT_RGBX888;
		pixel_size = 4;
		break;
	case DRM_FORMAT_YUYV:
		dvscntr |= DVS_FORMAT_YUV422 | DVS_YUV_ORDER_YUYV;
		pixel_size = 2;
		break;
	case DRM_FORMAT_YVYU:
		dvscntr |= DVS_FORMAT_YUV422 | DVS_YUV_ORDER_YVYU;
		pixel_size = 2;
		break;
	case DRM_FORMAT_UYVY:
		dvscntr |= DVS_FORMAT_YUV422 | DVS_YUV_ORDER_UYVY;
		pixel_size = 2;
		break;
	case DRM_FORMAT_VYUY:
		dvscntr |= DVS_FORMAT_YUV422 | DVS_YUV_ORDER_VYUY;
		pixel_size = 2;
		break;
	default:
		DRM_DEBUG_DRIVER("bad pixel format, assuming RGBX888\n");
		dvscntr |= DVS_FORMAT_RGBX888;
		pixel_size = 4;
		break;
	}

	if (obj->tiling_mode != I915_TILING_NONE)
		dvscntr |= DVS_TILED;

	if (IS_GEN6(dev))
		dvscntr |= DVS_TRICKLE_FEED_DISABLE; /* must disable */
	dvscntr |= DVS_ENABLE;

	/* Sizes are 0 based */
	src_w--;
	src_h--;
	crtc_w--;
	crtc_h--;

	intel_update_sprite_watermarks(dev, pipe, crtc_w, pixel_size);

	dvsscale = 0;
	if (IS_GEN5(dev) || crtc_w != src_w || crtc_h != src_h)
		dvsscale = DVS_SCALE_ENABLE | (src_w << 16) | src_h;

	I915_WRITE(DVSSTRIDE(pipe), fb->pitches[0]);
	I915_WRITE(DVSPOS(pipe), (crtc_y << 16) | crtc_x);

	linear_offset = y * fb->pitches[0] + x * (fb->bits_per_pixel / 8);
	dvssurf_offset =
		intel_gen4_compute_page_offset(&x, &y, obj->tiling_mode,
					       pixel_size, fb->pitches[0]);
	linear_offset -= dvssurf_offset;

	if (obj->tiling_mode != I915_TILING_NONE)
		I915_WRITE(DVSTILEOFF(pipe), (y << 16) | x);
	else
		I915_WRITE(DVSLINOFF(pipe), linear_offset);

	I915_WRITE(DVSSIZE(pipe), (crtc_h << 16) | crtc_w);
	I915_WRITE(DVSSCALE(pipe), dvsscale);
	I915_WRITE(DVSCNTR(pipe), dvscntr);
	I915_MODIFY_DISPBASE(DVSSURF(pipe), obj->gtt_offset + dvssurf_offset);
	POSTING_READ(DVSSURF(pipe));
}

static void
ilk_disable_plane(struct drm_plane *plane)
{
	struct drm_device *dev = plane->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_plane *intel_plane = to_intel_plane(plane);
	int pipe = intel_plane->pipe;

	I915_WRITE(DVSCNTR(pipe), I915_READ(DVSCNTR(pipe)) & ~DVS_ENABLE);
	/* Disable the scaler */
	I915_WRITE(DVSSCALE(pipe), 0);
	/* Flush double buffered register updates */
	I915_MODIFY_DISPBASE(DVSSURF(pipe), 0);
	POSTING_READ(DVSSURF(pipe));
}

static void
intel_enable_primary(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	int reg = DSPCNTR(intel_crtc->plane);

	if (!intel_crtc->primary_disabled)
		return;

	intel_crtc->primary_disabled = false;
	intel_update_fbc(dev);

	I915_WRITE(reg, I915_READ(reg) | DISPLAY_PLANE_ENABLE);
}

static void
intel_disable_primary(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	int reg = DSPCNTR(intel_crtc->plane);

	if (intel_crtc->primary_disabled)
		return;

	I915_WRITE(reg, I915_READ(reg) & ~DISPLAY_PLANE_ENABLE);

	intel_crtc->primary_disabled = true;
	intel_update_fbc(dev);
}

static int
ilk_update_colorkey(struct drm_plane *plane,
		    struct drm_intel_sprite_colorkey *key)
{
	struct drm_device *dev = plane->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_plane *intel_plane;
	u32 dvscntr;
	int ret = 0;

	intel_plane = to_intel_plane(plane);

	I915_WRITE(DVSKEYVAL(intel_plane->pipe), key->min_value);
	I915_WRITE(DVSKEYMAX(intel_plane->pipe), key->max_value);
	I915_WRITE(DVSKEYMSK(intel_plane->pipe), key->channel_mask);

	dvscntr = I915_READ(DVSCNTR(intel_plane->pipe));
	dvscntr &= ~(DVS_SOURCE_KEY | DVS_DEST_KEY);
	if (key->flags & I915_SET_COLORKEY_DESTINATION)
		dvscntr |= DVS_DEST_KEY;
	else if (key->flags & I915_SET_COLORKEY_SOURCE)
		dvscntr |= DVS_SOURCE_KEY;
	I915_WRITE(DVSCNTR(intel_plane->pipe), dvscntr);

	POSTING_READ(DVSKEYMSK(intel_plane->pipe));

	return ret;
}

static void
ilk_get_colorkey(struct drm_plane *plane, struct drm_intel_sprite_colorkey *key)
{
	struct drm_device *dev = plane->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_plane *intel_plane;
	u32 dvscntr;

	intel_plane = to_intel_plane(plane);

	key->min_value = I915_READ(DVSKEYVAL(intel_plane->pipe));
	key->max_value = I915_READ(DVSKEYMAX(intel_plane->pipe));
	key->channel_mask = I915_READ(DVSKEYMSK(intel_plane->pipe));
	key->flags = 0;

	dvscntr = I915_READ(DVSCNTR(intel_plane->pipe));

	if (dvscntr & DVS_DEST_KEY)
		key->flags = I915_SET_COLORKEY_DESTINATION;
	else if (dvscntr & DVS_SOURCE_KEY)
		key->flags = I915_SET_COLORKEY_SOURCE;
	else
		key->flags = I915_SET_COLORKEY_NONE;
}

static int
intel_update_plane(struct drm_plane *plane, struct drm_crtc *crtc,
		   struct drm_framebuffer *fb, int crtc_x, int crtc_y,
		   unsigned int crtc_w, unsigned int crtc_h,
		   uint32_t src_x, uint32_t src_y,
		   uint32_t src_w, uint32_t src_h)
{
	struct drm_device *dev = plane->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	struct intel_plane *intel_plane = to_intel_plane(plane);
	struct intel_framebuffer *intel_fb;
	struct drm_i915_gem_object *obj, *old_obj;
	int pipe = intel_plane->pipe;
	int ret = 0;
	int x = src_x >> 16, y = src_y >> 16;
	int primary_w = crtc->mode.hdisplay, primary_h = crtc->mode.vdisplay;
	bool disable_primary = false;

	intel_fb = to_intel_framebuffer(fb);
	obj = intel_fb->obj;

	old_obj = intel_plane->obj;

	src_w = src_w >> 16;
	src_h = src_h >> 16;

	/* Pipe must be running... */
	if (!(I915_READ(PIPECONF(pipe)) & PIPECONF_ENABLE))
		return -EINVAL;

	if (crtc_x >= primary_w || crtc_y >= primary_h)
		return -EINVAL;

	/* Don't modify another pipe's plane */
	if (intel_plane->pipe != intel_crtc->pipe)
		return -EINVAL;

	/*
	 * Clamp the width & height into the visible area.  Note we don't
	 * try to scale the source if part of the visible region is offscreen.
	 * The caller must handle that by adjusting source offset and size.
	 */
	if ((crtc_x < 0) && ((crtc_x + crtc_w) > 0)) {
		crtc_w += crtc_x;
		crtc_x = 0;
	}
	if ((crtc_x + crtc_w) <= 0) /* Nothing to display */
		goto out;
	if ((crtc_x + crtc_w) > primary_w)
		crtc_w = primary_w - crtc_x;

	if ((crtc_y < 0) && ((crtc_y + crtc_h) > 0)) {
		crtc_h += crtc_y;
		crtc_y = 0;
	}
	if ((crtc_y + crtc_h) <= 0) /* Nothing to display */
		goto out;
	if (crtc_y + crtc_h > primary_h)
		crtc_h = primary_h - crtc_y;

	if (!crtc_w || !crtc_h) /* Again, nothing to display */
		goto out;

	/*
	 * If the sprite is completely covering the primary plane,
	 * we can disable the primary and save power.
	 */
	if (!IS_VALLEYVIEW(dev)) {
		if ((crtc_x == 0) && (crtc_y == 0) &&
		(crtc_w == primary_w) && (crtc_h == primary_h))
			disable_primary = true;
	}

	mutex_lock(&dev->struct_mutex);

	ret = intel_pin_and_fence_fb_obj(dev, obj, NULL);
	if (ret)
		goto out_unlock;

	intel_plane->obj = obj;

	/*
	 * Be sure to re-enable the primary before the sprite is no longer
	 * covering it fully.
	 */
	if (!IS_VALLEYVIEW(dev)) {
		if (!disable_primary)
			intel_enable_primary(crtc);
	}

	intel_plane->update_plane(plane, fb, obj, crtc_x, crtc_y,
				  crtc_w, crtc_h, x, y, src_w, src_h);

	if (!IS_VALLEYVIEW(dev)) {
		if (disable_primary)
			intel_disable_primary(crtc);
	}

	/* Unpin old obj after new one is active to avoid ugliness */
	if (old_obj) {
		if (!IS_VALLEYVIEW(dev)) {
			/*
			 * It's fairly common to simply update the position of
			 * an existing object.  In that case, we don't need to
			 * wait for vblank to avoid ugliness, we only need to
			 * do the pin & ref bookkeeping.
			 */
			if (old_obj != obj) {
				mutex_unlock(&dev->struct_mutex);
				intel_wait_for_vblank(dev,
					to_intel_crtc(crtc)->pipe);
				mutex_lock(&dev->struct_mutex);
			}
		}
		intel_unpin_fb_obj(old_obj);
	}

out_unlock:
	mutex_unlock(&dev->struct_mutex);
out:
	return ret;
}

static int
intel_disable_plane(struct drm_plane *plane)
{
	struct drm_device *dev = plane->dev;
	struct intel_plane *intel_plane = to_intel_plane(plane);
	int ret = 0;

	if (plane->crtc)
		intel_enable_primary(plane->crtc);

	intel_plane->disable_plane(plane);

	if (!intel_plane->obj)
		goto out;

	mutex_lock(&dev->struct_mutex);
	intel_unpin_fb_obj(intel_plane->obj);
	intel_plane->obj = NULL;
	mutex_unlock(&dev->struct_mutex);
out:

	return ret;
}

static void intel_destroy_plane(struct drm_plane *plane)
{
	struct intel_plane *intel_plane = to_intel_plane(plane);
	intel_disable_plane(plane);
	drm_plane_cleanup(plane);
	kfree(intel_plane);
}

int intel_sprite_set_colorkey(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	struct drm_intel_sprite_colorkey *set = data;
	struct drm_mode_object *obj;
	struct drm_plane *plane;
	struct intel_plane *intel_plane;
	int ret = 0;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -ENODEV;

	/* Make sure we don't try to enable both src & dest simultaneously */
	if ((set->flags & (I915_SET_COLORKEY_DESTINATION | I915_SET_COLORKEY_SOURCE)) == (I915_SET_COLORKEY_DESTINATION | I915_SET_COLORKEY_SOURCE))
		return -EINVAL;

	mutex_lock(&dev->mode_config.mutex);

	obj = drm_mode_object_find(dev, set->plane_id, DRM_MODE_OBJECT_PLANE);
	if (!obj) {
		ret = -EINVAL;
		goto out_unlock;
	}

	plane = obj_to_plane(obj);
	intel_plane = to_intel_plane(plane);
	ret = intel_plane->update_colorkey(plane, set);

out_unlock:
	mutex_unlock(&dev->mode_config.mutex);
	return ret;
}

int intel_sprite_get_colorkey(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	struct drm_intel_sprite_colorkey *get = data;
	struct drm_mode_object *obj;
	struct drm_plane *plane;
	struct intel_plane *intel_plane;
	int ret = 0;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -ENODEV;

	mutex_lock(&dev->mode_config.mutex);

	obj = drm_mode_object_find(dev, get->plane_id, DRM_MODE_OBJECT_PLANE);
	if (!obj) {
		ret = -EINVAL;
		goto out_unlock;
	}

	plane = obj_to_plane(obj);
	intel_plane = to_intel_plane(plane);
	intel_plane->get_colorkey(plane, get);

out_unlock:
	mutex_unlock(&dev->mode_config.mutex);
	return ret;
}

static const struct drm_plane_funcs intel_plane_funcs = {
	.update_plane = intel_update_plane,
	.disable_plane = intel_disable_plane,
	.destroy = intel_destroy_plane,
};

static uint32_t ilk_plane_formats[] = {
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_YUYV,
	DRM_FORMAT_YVYU,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_VYUY,
};

static uint32_t snb_plane_formats[] = {
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_YUYV,
	DRM_FORMAT_YVYU,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_VYUY,
};

static uint32_t vlv_plane_formats[] = {
	DRM_FORMAT_RGB565,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR2101010,
	DRM_FORMAT_ABGR2101010,
	DRM_FORMAT_YUYV,
	DRM_FORMAT_YVYU,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_VYUY,
};


int
intel_plane_init(struct drm_device *dev, enum pipe pipe, int plane)
{
	struct intel_plane *intel_plane;
	unsigned long possible_crtcs;
	const uint32_t *plane_formats;
	int num_plane_formats;
	int ret;

	if (INTEL_INFO(dev)->gen < 5)
		return -ENODEV;

	intel_plane = kzalloc(sizeof(struct intel_plane), GFP_KERNEL);
	if (!intel_plane)
		return -ENOMEM;

	switch (INTEL_INFO(dev)->gen) {
	case 5:
	case 6:
		intel_plane->max_downscale = 16;
		intel_plane->update_plane = ilk_update_plane;
		intel_plane->disable_plane = ilk_disable_plane;
		intel_plane->update_colorkey = ilk_update_colorkey;
		intel_plane->get_colorkey = ilk_get_colorkey;

		if (IS_GEN6(dev)) {
			plane_formats = snb_plane_formats;
			num_plane_formats = ARRAY_SIZE(snb_plane_formats);
		} else {
			plane_formats = ilk_plane_formats;
			num_plane_formats = ARRAY_SIZE(ilk_plane_formats);
		}
		break;

	case 7:
		if (IS_IVYBRIDGE(dev)) {
			intel_plane->max_downscale = 2;
			intel_plane->update_plane = ivb_update_plane;
			intel_plane->disable_plane = ivb_disable_plane;
			intel_plane->update_colorkey = ivb_update_colorkey;
			intel_plane->get_colorkey = ivb_get_colorkey;

			plane_formats = snb_plane_formats;
			num_plane_formats = ARRAY_SIZE(snb_plane_formats);
		} else if (IS_VALLEYVIEW(dev)) {
			intel_plane->max_downscale = 1;
			intel_plane->update_plane = vlv_update_plane;
			intel_plane->disable_plane = vlv_disable_plane;
			intel_plane->update_colorkey = vlv_update_colorkey;
			intel_plane->get_colorkey = vlv_get_colorkey;
			plane_formats = vlv_plane_formats;
			num_plane_formats = ARRAY_SIZE(vlv_plane_formats);
		} else {
			ret = -ENODEV;
			goto err_free;
		}
		break;

	default:
		ret = -ENODEV;
		goto err_free;
	}

	intel_plane->pipe = pipe;
	intel_plane->plane = plane;
	possible_crtcs = (1 << pipe);
	ret = drm_plane_init(dev, &intel_plane->base, possible_crtcs,
			     &intel_plane_funcs,
			     plane_formats, num_plane_formats,
			     false);
	if (ret)
		goto err_free;

	return ret;

err_free:
	kfree(intel_plane);
	return ret;
}
