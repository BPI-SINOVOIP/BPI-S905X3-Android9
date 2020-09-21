Stub Annotations

The annotations in these packages are used to compile
the stubs. They are mostly identical to the annotations
in the support library, but

(1) There are some annotations here which are not in
   the support library, such as @RecentlyNullable and
   @RecentlyNonNull. These are used *only* in the stubs
   to automatically mark code as recently annotated
   with null/non-null. We do *not* want these annotations
   in the source code; the recent-ness is computed at
   build time and injected into the stubs in place
   of the normal null annotations.

(2) There are some annotations in the support library
   that do not apply here, such as @Keep,
   @VisibleForTesting, etc.

(3) Only class retention annotations are interesting for
   the stubs; that means for example that we don't
   include @IntDef and @StringDef.

(4) We've tweaked the retention of some of the support
   library annotations; some of them were accidentally
   source retention, and here they are class retention.

(5) We've tweaked the nullness annotations to include
   TYPE_PARAMETER and TYPE_USE targets, and removed
   package, local variable, annotation type, etc.

(6) There are some other differences; for example, the
   @RequiresPermission annotation here has an extra
   "apis" field used for merged historical annotations
   to express the applicable API range.

Essentially, this is a curated list of exactly the
set of annotations we allow injected into the stubs.

