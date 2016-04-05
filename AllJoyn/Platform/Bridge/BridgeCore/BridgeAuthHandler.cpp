//
// Copyright (c) 2015, Microsoft Corporation
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
// IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//

#include "pch.h"
#include "BridgeCommon.h"
#include "BridgeAuthHandler.h"

#include <alljoyn_c/version.h>
#include <alljoyn_c/Status.h>

using namespace Bridge;

static const char KEYX_AUTHENTICATION[] = "ALLJOYN_SRP_KEYX";
static const char CREDENTIAL_AUTHENTICATION[] = "ALLJOYN_SRP_LOGON";
static const char ECDHE_ECDSA_AUTHENTICATION[] = "ALLJOYN_ECDHE_ECDSA";

BridgeAuthHandler::BridgeAuthHandler() :
    m_authListener(nullptr),
    m_busAttachment(nullptr)
{
}

BridgeAuthHandler::~BridgeAuthHandler()
{
}

_Use_decl_annotations_
QStatus
BridgeAuthHandler::InitializeWithAllAuthenticationMethods(_In_ alljoyn_busattachment bus,
    _In_ std::string keyx,
    _In_ std::string username, _In_ std::string password,
    _In_ std::string ecdheEcdsaPrivateKey, _In_ std::string ecdheEcdsaCertChain)
{
    QStatus status = ER_OK;
    std::string authMechanisms = "";

    /* Auth listener callbacks. */
    alljoyn_authlistenerasync_callbacks authcbs =
    {
        (alljoyn_authlistener_requestcredentialsasync_ptr) BridgeAuthHandler::ReqCredentialsAsync,
        BridgeAuthHandler::VerifyCredentialsAsync,
        nullptr,
        BridgeAuthHandler::AuthComplete
    };

    // sanity check
    if (nullptr == bus)
    {
        status = ER_BAD_ARG_1;
        goto Leave;
    }

    m_busAttachment = bus;
    alljoyn_busattachment_clearkeystore(m_busAttachment);

    // save credentials
    if (!keyx.empty())
    {
        m_keyx = keyx;
    }
    else
    {
        m_keyx.clear();
    }

    if (!username.empty())
    {
        m_username = username;
    }
    else
    {
        m_username.clear();
    }

    if (!password.empty())
    {
        m_password = password;
    }
    else
    {
        m_password.clear();
    }

    if (!ecdheEcdsaPrivateKey.empty())
    {
        m_ecdheEcdsaPrivateKey = ecdheEcdsaPrivateKey;
    }
    else
    {
        m_ecdheEcdsaPrivateKey.clear();
    }

    if (!ecdheEcdsaCertChain.empty())
    {
        m_ecdheEcdsaCertChain = ecdheEcdsaCertChain;
    }
    else
    {
        m_ecdheEcdsaCertChain.clear();
    }

    // build supported authentication mechanism
    if (!m_keyx.empty())
    {
        authMechanisms = KEYX_AUTHENTICATION;
    }

    if (0 != m_username.length() &&
        0 != m_password.length())
    {
        if (0 != authMechanisms.length())
        {
            authMechanisms += " ";
        }
        authMechanisms += CREDENTIAL_AUTHENTICATION;
    }

    if (0 != m_ecdheEcdsaPrivateKey.length() &&
        0 != m_ecdheEcdsaCertChain.length())
    {
        if (0 != authMechanisms.length())
        {
            authMechanisms += " ";
        }
        authMechanisms += ECDHE_ECDSA_AUTHENTICATION;
    }

    // verify there at least 1 authentication mechanism
    if (0 == authMechanisms.length())
    {
        status = ER_BAD_ARG_2;
        goto Leave;
    }


    /* Auth listener callbacks. */
    m_authListener = alljoyn_authlistenerasync_create(&authcbs, this);

    status = alljoyn_busattachment_enablepeersecurity(m_busAttachment, authMechanisms.c_str(), m_authListener, nullptr, QCC_FALSE);
    if (ER_OK != status)
    {
        goto Leave;
    }

    // register username/password in key store if necessary
    if (0 != m_username.length() &&
        0 != m_password.length())
    {
        status = alljoyn_busattachment_addlogonentry(m_busAttachment, CREDENTIAL_AUTHENTICATION, m_username.c_str(), m_password.c_str());
        if (ER_OK != status)
        {
            goto Leave;
        }
    }

Leave:
    if (ER_OK != status)
    {
        ShutDown();
    }
    return ER_OK;
}

void BridgeAuthHandler::ResetAccess(_In_ std::string remoteName)
{
    QStatus status = ER_OK;
    size_t peerGuidSize = 0;

    // sanity check
    if (0 == remoteName.length() ||
        nullptr == m_authListener ||
        nullptr == m_busAttachment)
    {
        goto leave;
    }

    // get peer Guid and reset access authorization
    status = alljoyn_busattachment_getpeerguid(m_busAttachment, remoteName.c_str(), nullptr, &peerGuidSize);
    if (ER_OK == status &&
        0 != peerGuidSize)
    {
        char *tempPeerGuid = new(std::nothrow) char[peerGuidSize + 1];
        if (nullptr != tempPeerGuid)
        {
            memset(tempPeerGuid, 0, peerGuidSize + 1);
            alljoyn_busattachment_getpeerguid(m_busAttachment, remoteName.c_str(), tempPeerGuid, &peerGuidSize);
            alljoyn_busattachment_clearkeys(m_busAttachment, tempPeerGuid);
            delete[] tempPeerGuid;
        }
    }

leave:
    return;
}

void BridgeAuthHandler::ShutDown()
{
    if (m_authListener != nullptr)
    {
        alljoyn_busattachment_enablepeersecurity(m_busAttachment, nullptr, m_authListener, nullptr, QCC_FALSE);
        alljoyn_authlistener_destroy(m_authListener);
        alljoyn_busattachment_clearkeystore(m_busAttachment);

        m_authListener = nullptr;
    }

    m_busAttachment = nullptr;
    m_keyx.clear();
    m_username.clear();
    m_password.clear();
    m_ecdheEcdsaPrivateKey.clear();
    m_ecdheEcdsaCertChain.clear();
}

_Use_decl_annotations_
QStatus AJ_CALL
BridgeAuthHandler::ReqCredentialsAsync(
    void* context,
    alljoyn_authlistener listener,
    const char* authMechanism,
    const char* /*authPeer*/,
    uint16_t /*authCount*/,
    const char* userId,
    uint16_t credMask,
    void* authContext)
{
    BridgeAuthHandler* pThis = reinterpret_cast<BridgeAuthHandler*>(context);
    QStatus status = ER_OK;
    std::string username;

    alljoyn_credentials creds = alljoyn_credentials_create();
    if (nullptr != userId)
    {
        username = userId;
    }

    if ((strcmp(authMechanism, KEYX_AUTHENTICATION) == 0) &&
        ((credMask & ALLJOYN_CRED_PASSWORD) == ALLJOYN_CRED_PASSWORD))
    {
        // KEYX authentication
        alljoyn_credentials_setpassword(creds, pThis->m_keyx.c_str());
        status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
    }
    else if ((strcmp(authMechanism, CREDENTIAL_AUTHENTICATION) == 0) &&
        ((credMask & ALLJOYN_CRED_PASSWORD) == ALLJOYN_CRED_PASSWORD) &&
         username == pThis->m_username)
    {
        // username/password authentication (and username match)
        alljoyn_credentials_setpassword(creds, pThis->m_password.c_str());
        status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
    }
    else if ((strcmp(authMechanism, ECDHE_ECDSA_AUTHENTICATION) == 0))
    {
        // set ECDHE ECDSA private key and CERT chain
        if ((credMask & ALLJOYN_CRED_PRIVATE_KEY) == ALLJOYN_CRED_PRIVATE_KEY)
        {
            alljoyn_credentials_setprivatekey(creds, pThis->m_ecdheEcdsaPrivateKey.c_str());
        }
        if ((credMask & ALLJOYN_CRED_CERT_CHAIN) == ALLJOYN_CRED_CERT_CHAIN)
        {
            alljoyn_credentials_setcertchain(creds, pThis->m_ecdheEcdsaCertChain.c_str());
        }
        status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
    }
    else
    {
        status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_FALSE, creds);
    }

    alljoyn_credentials_destroy(creds);
    return status;
}

_Use_decl_annotations_
QStatus AJ_CALL
BridgeAuthHandler::VerifyCredentialsAsync(
    const void * /*context*/,
    alljoyn_authlistener listener,
    const char * authMechanism,
    const char * /*authPeer*/,
    const alljoyn_credentials creds,
    void * authContext)
{
    QStatus status = ER_OK;
    bool isVerified = false;

    if (strcmp(authMechanism, ECDHE_ECDSA_AUTHENTICATION) == 0)
    {
        if (alljoyn_credentials_isset(creds, ALLJOYN_CRED_CERT_CHAIN))
        {
            // get the cert chain to verify
            /*const char *cert =*/ alljoyn_credentials_getcertchain(creds);
            isVerified = true;
        }
    }

    if (isVerified)
    {
        status = alljoyn_authlistener_verifycredentialsresponse(listener, authContext, QCC_TRUE);
    }
    else
    {
        status = alljoyn_authlistener_verifycredentialsresponse(listener, authContext, QCC_FALSE);
    }

    return status;
}

_Use_decl_annotations_
void AJ_CALL
BridgeAuthHandler::AuthComplete(const void* /*context*/, const char* /*authMechanism*/, const char* /*authPeer*/, QCC_BOOL /*success*/)
{
}
