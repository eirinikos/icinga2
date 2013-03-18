/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef TLSUTILITY_H
#define TLSUTILITY_H

#include "base/i2-base.h"
#include "base/object.h"
#include "base/qstring.h"
#include "base/exception.h"
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace icinga
{

shared_ptr<SSL_CTX> MakeSSLContext(const String& pubkey, const String& privkey, const String& cakey);
String GetCertificateCN(const shared_ptr<X509>& certificate);
shared_ptr<X509> GetX509Certificate(const String& pemfile);

class I2_BASE_API openssl_error : virtual public std::exception, virtual public boost::exception { };

struct errinfo_openssl_error_;
typedef boost::error_info<struct errinfo_openssl_error_, int> errinfo_openssl_error;

inline std::string to_string(const errinfo_openssl_error& e)
{
        std::ostringstream tmp;
        int code = e.value();

        const char *message = ERR_error_string(code, NULL);

        if (message == NULL)
                message = "Unknown error.";

        tmp << code << ", \"" << message << "\"";
        return tmp.str();
}

}

#endif /* TLSUTILITY_H */
