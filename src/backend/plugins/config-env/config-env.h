/* config-env.h
 *
 * Copyright 2022-2023 Jan-Michael Brummer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <glib.h>
#include <libpeas/peas.h>

G_BEGIN_DECLS

#define PX_CONFIG_TYPE_ENV         (px_config_env_get_type ())
#define PX_CONFIG_ENV(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), PX_CONFIG_TYPE_ENV, PxConfigEnv))
#define PX_CONFIG_ENV_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), PX_CONFIG_TYPE_ENV, PxConfigEnv))
#define PX_CONFIG_IS_ENV(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), PX_CONFIG_TYPE_ENV))
#define PX_CONFIG_IS_ENV_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), PX_CONFIG_TYPE_ENV))
#define PX_CONFIG_ENV_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), PX_CONFIG_TYPE_ENV, PxConfigEnv))

typedef struct _PxConfigEnv PxConfigEnv;
typedef struct _PxConfigEnvClass PxConfigEnvClass;

struct _PxConfigEnv {
    PeasExtensionBase parent_instance;
};

struct _PxConfigEnvClass {
    PeasExtensionBaseClass parent_class;
};

GType                 px_config_env_get_type        (void) G_GNUC_CONST;

G_MODULE_EXPORT void  peas_register_types                         (PeasObjectModule *module);

G_END_DECLS


