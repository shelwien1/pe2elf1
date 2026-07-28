/* xad_idxparam.inc -- part of xadpcm.cpp; not standalone.
 *
 * the bridge between the generated IDX headers and the C++ that uses them.
 * Included AFTER MOD/*.inc, and it has to be -- it switches on USE_NEW, which
 * those headers are what define.  That is the cycle xad_prelude.inc exists
 * separately for: the generated headers need pclamp and the contract
 * constants, and this needs the generated headers.  Do not merge the two. */

#define NCONF pclamp(G0_NCONF, 1, 16)

/* IDX values as TEMPLATE PARAMETERS, in both build modes.
 *
 * In the Const build idx2inc.pl has already folded every parameter to a
 * literal, so `G0_MATCH_ORDER` is an integer constant expression and can be a
 * template argument like any other.  In the Debug build it is
 * `G0_MATCH_ORDER_.value * 1` off a mapping object that opt.pl patches at load
 * time -- not a constant expression, and therefore not a template argument at
 * all.  That is what kept these values out of parameter lists.
 *
 * A non-type parameter of REFERENCE type settles it: `const int&` binds an
 * object with static storage and internal linkage whatever its initialiser
 * was, so `MatchModel<G0_MATCH_ORDER, ...>` is well-formed in both builds and
 * spelled identically at the instantiation site.  Only the parameter's
 * declared type differs, and only the Const build folds -- which is the build
 * that ships.
 *
 *   IDXP(name)        declares such a parameter
 *   IDXC(t, n, expr)  declares a class constant derived from one: a
 *                     static constexpr where that is possible, a const member
 *                     initialised at construction where it is not
 *
 * USE_NEW is emitted by idx2inc.pl itself into every generated _h.inc, and
 * `idx2inc.pl foo.idx 0` is documented there as the mode whose Table() members
 * are fixed-size arrays -- i.e. the one that needs const-expression sizes.
 * mk.sh follows that convention (Debug -> 1, release -> 0), so the flag travels
 * WITH the generated headers instead of on the compiler command line, where it
 * could drift out of step with whatever is actually in MOD/. */
/* Instantiating on an IDX value gives the instantiated type INTERNAL linkage,
 * because the generated parameters are `static const int`.  -Wsubobject-linkage
 * warns about a member of such a type -- correctly, for a class declared in a
 * header that several translation units each get their own copy of, since
 * those copies would be different types under one name.  xadpcm is one
 * translation unit by construction (xadpcm.cpp includes every part; there is
 * nothing to link against), so there is no second copy for this one to
 * disagree with and the diagnostic has nothing to catch. */
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
#endif

#if USE_NEW
 #define IDXP(name)       const int& name
 #define IDXC(t, n, expr) const t n = (expr)
#else
 #define IDXP(name)       int name
 #define IDXC(t, n, expr) static constexpr t n = (expr)
/* Backstop, because the coupling above is mk.sh's convention and not a law: if
 * MOD/ says USE_NEW=0 while actually holding Debug-mode mapping objects, these
 * operands are not constant expressions and the build stops here rather than
 * silently mis-templating.  The failure is one-sided by design -- the other
 * mistake (Const values, reference parameters) compiles and runs correctly,
 * just without the folding. */
static_assert(G0_MATCH_ORDER>=0&&G0_MLR>=0&&M0_Md_Volume>0&&I0_Ib_Volume>0,
              "MOD/ declares USE_NEW=0 but its parameters are not constant "
              "expressions -- regenerate it with ./mk.sh release");
#endif
