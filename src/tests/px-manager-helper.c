#include "px-manager.h"
#include "px-manager-helper.h"

PxManager *
px_test_manager_new (void)
{
  g_autofree char *path = g_strdup_printf ("%s/../backend/modules", g_getenv ("G_TEST_BUILDDIR"));

  return g_object_new (PX_TYPE_MANAGER, "module-path", path, NULL);
}
