/**
 * @cond internal
 * @file decaf_crypto.c
 * @copyright
 *   Copyright (c) 2015 Cryptography Research, Inc.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 * @author Mike Hamburg
 * @brief Example Decaf cyrpto routines.
 */

#include <decaf/crypto.h>
#include <string.h>

static const unsigned int DECAF_255_SCALAR_OVERKILL_BYTES = DECAF_255_SCALAR_BYTES + 8;

void decaf_255_derive_private_key (
    decaf_255_private_key_t priv,
    const decaf_255_symmetric_key_t proto
) {
    const char *magic = "decaf_255_derive_private_key";
    uint8_t encoded_scalar[DECAF_255_SCALAR_OVERKILL_BYTES];
    decaf_255_point_t pub;

    shake256_ctx_t sponge;
    shake256_init(sponge);
    shake256_update(sponge, proto, sizeof(decaf_255_symmetric_key_t));
    shake256_update(sponge, (const unsigned char *)magic, strlen(magic));
    shake256_final(sponge, encoded_scalar, sizeof(encoded_scalar));
    shake256_destroy(sponge);
    
    memcpy(priv->sym, proto, sizeof(decaf_255_symmetric_key_t));
    decaf_255_scalar_decode_long(priv->secret_scalar, encoded_scalar, sizeof(encoded_scalar));
    
    decaf_255_precomputed_scalarmul(pub, decaf_255_precomputed_base, priv->secret_scalar);
    decaf_255_point_encode(priv->pub, pub);
    
    decaf_bzero(encoded_scalar, sizeof(encoded_scalar));
}

void
decaf_255_destroy_private_key (
    decaf_255_private_key_t priv
)  {
    decaf_bzero((void*)priv, sizeof(decaf_255_private_key_t));
}

void decaf_255_private_to_public (
    decaf_255_public_key_t pub,
    const decaf_255_private_key_t priv
) {
    memcpy(pub, priv->pub, sizeof(decaf_255_public_key_t));
}

decaf_error_t
decaf_255_shared_secret (
    uint8_t *shared,
    size_t shared_bytes,
    const decaf_255_private_key_t my_privkey,
    const decaf_255_public_key_t your_pubkey
) {
    uint8_t ss_ser[DECAF_255_SER_BYTES];
    const char *nope = "decaf_255_ss_invalid";
    
    unsigned i;
    /* Lexsort keys.  Less will be -1 if mine is less, and 0 otherwise. */
    uint16_t less = 0;
    for (i=0; i<DECAF_255_SER_BYTES; i++) {
        uint16_t delta = my_privkey->pub[i];
        delta -= your_pubkey[i];
        /* Case:
         * = -> delta = 0 -> hi delta-1 = -1, hi delta = 0
         * > -> delta > 0 -> hi delta-1 = 0, hi delta = 0
         * < -> delta < 0 -> hi delta-1 = (doesnt matter), hi delta = -1
         */
        less &= delta-1;
        less |= delta;
    }
    less >>= 8;

    shake256_ctx_t sponge;
    shake256_init(sponge);

    /* update the lesser */
    for (i=0; i<sizeof(ss_ser); i++) {
        ss_ser[i] = (my_privkey->pub[i] & less) | (your_pubkey[i] & ~less);
    }
    shake256_update(sponge, ss_ser, sizeof(ss_ser));

    /* update the greater */
    for (i=0; i<sizeof(ss_ser); i++) {
        ss_ser[i] = (my_privkey->pub[i] & ~less) | (your_pubkey[i] & less);
    }
    shake256_update(sponge, ss_ser, sizeof(ss_ser));
    
    decaf_error_t ret = decaf_255_direct_scalarmul(ss_ser, your_pubkey, my_privkey->secret_scalar, DECAF_FALSE, DECAF_TRUE);
    decaf_bool_t good = decaf_successful(ret);
    /* If invalid, then replace ... */
    for (i=0; i<sizeof(ss_ser); i++) {
        ss_ser[i] &= good;
        
        if (i < sizeof(my_privkey->sym)) {
            ss_ser[i] |= my_privkey->sym[i] & ~good;
        } else if (i - sizeof(my_privkey->sym) < strlen(nope)) {
            ss_ser[i] |= nope[i-sizeof(my_privkey->sym)] & ~good;
        }
    }

    shake256_update(sponge, ss_ser, sizeof(ss_ser));
    shake256_final(sponge, shared, shared_bytes);
    shake256_destroy(sponge);
    
    decaf_bzero(ss_ser, sizeof(ss_ser));
    
    return ret;
}

void
decaf_255_sign_shake (
    decaf_255_signature_t sig,
    const decaf_255_private_key_t priv,
    const shake256_ctx_t shake
) {
    const char *magic = "decaf_255_sign_shake";

    uint8_t overkill[DECAF_255_SCALAR_OVERKILL_BYTES], encoded[DECAF_255_SER_BYTES];
    decaf_255_point_t point;
    decaf_255_scalar_t nonce, challenge;
    
    /* Derive nonce */
    shake256_ctx_t ctx;
    memcpy(ctx, shake, sizeof(ctx));
    shake256_update(ctx, priv->sym, sizeof(priv->sym));
    shake256_update(ctx, (const unsigned char *)magic, strlen(magic));
    shake256_final(ctx, overkill, sizeof(overkill));
    
    decaf_255_scalar_decode_long(nonce, overkill, sizeof(overkill));
    decaf_255_precomputed_scalarmul(point, decaf_255_precomputed_base, nonce);
    decaf_255_point_encode(encoded, point);

    /* Derive challenge */
    memcpy(ctx, shake, sizeof(ctx));
    shake256_update(ctx, priv->pub, sizeof(priv->pub));
    shake256_update(ctx, encoded, sizeof(encoded));
    shake256_final(ctx, overkill, sizeof(overkill));
    shake256_destroy(ctx);
    decaf_255_scalar_decode_long(challenge, overkill, sizeof(overkill));
    
    /* Respond */
    decaf_255_scalar_mul(challenge, challenge, priv->secret_scalar);
    decaf_255_scalar_sub(nonce, nonce, challenge);
    
    /* Save results */
    memcpy(sig, encoded, sizeof(encoded));
    decaf_255_scalar_encode(&sig[sizeof(encoded)], nonce);
    
    /* Clean up */
    decaf_255_scalar_destroy(nonce);
    decaf_255_scalar_destroy(challenge);
    decaf_bzero(overkill,sizeof(overkill));
    decaf_bzero(encoded,sizeof(encoded));
}

decaf_error_t
decaf_255_verify_shake (
    const decaf_255_signature_t sig,
    const decaf_255_public_key_t pub,
    const shake256_ctx_t shake
) {
    decaf_bool_t ret;

    uint8_t overkill[DECAF_255_SCALAR_OVERKILL_BYTES];
    decaf_255_point_t point, pubpoint;
    decaf_255_scalar_t challenge, response;
    
    /* Derive challenge */
    shake256_ctx_t ctx;
    memcpy(ctx, shake, sizeof(ctx));
    shake256_update(ctx, pub, sizeof(decaf_255_public_key_t));
    shake256_update(ctx, sig, DECAF_255_SER_BYTES);
    shake256_final(ctx, overkill, sizeof(overkill));
    shake256_destroy(ctx);
    decaf_255_scalar_decode_long(challenge, overkill, sizeof(overkill));

    /* Decode points. */
    ret  = decaf_successful(decaf_255_point_decode(point, sig, DECAF_TRUE));
    ret &= decaf_successful(decaf_255_point_decode(pubpoint, pub, DECAF_FALSE));
    ret &= decaf_successful(decaf_255_scalar_decode(response, &sig[DECAF_255_SER_BYTES]));

    decaf_255_base_double_scalarmul_non_secret (
        pubpoint, response, pubpoint, challenge
    );

    ret &= decaf_255_point_eq(pubpoint, point);
    
    return decaf_succeed_if(ret);
}

void
decaf_255_sign (
    decaf_255_signature_t sig,
    const decaf_255_private_key_t priv,
    const unsigned char *message,
    size_t message_len
) {
    shake256_ctx_t ctx;
    shake256_init(ctx);
    shake256_update(ctx, message, message_len);
    decaf_255_sign_shake(sig, priv, ctx);
    shake256_destroy(ctx);
}

decaf_error_t
decaf_255_verify (
    const decaf_255_signature_t sig,
    const decaf_255_public_key_t pub,
    const unsigned char *message,
    size_t message_len
) {
    shake256_ctx_t ctx;
    shake256_init(ctx);
    shake256_update(ctx, message, message_len);
    decaf_error_t ret = decaf_255_verify_shake(sig, pub, ctx);
    shake256_destroy(ctx);
    return ret;
}
