/* See LICENSE file for copyright and license details. */
#include <stdint.h>

#if defined(HAVE_ENDIAN_H)
# include <endian.h>
#elif defined(HAVE_SYS_ENDIAN_H)
# include <sys/endian.h>
#endif

#if !defined(HAVE_ENDIAN_H) && !defined(HAVE_SYS_ENDIAN_H)

# if !defined(htole16)
#  define htole16 blind_htole16
static inline uint16_t
blind_htole16(uint16_t h)
{
	union {
		unsigned char bytes[2];
		uint16_t value;
	} d;
	d.bytes[0] = h;
	d.bytes[1] = h >> 8;
	return d.value;
}
# endif

# if !defined(le16toh)
#  if defined(letoh16)
#   define le16toh letoh16
#  else
#   define le16toh blind_le16toh
static inline uint16_t
blind_le16toh(uint16_t le)
{
	unsigned char *bytes = (unsigned char *)&le;
	return ((uint16_t)(bytes[1]) << 8) | (uint16_t)(bytes[0]);
}
#  endif
# endif

#elif defined(HAVE_OPENBSD_ENDIAN)
# define le16toh letoh16
#endif
