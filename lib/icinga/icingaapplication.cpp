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

#include "icinga/icingaapplication.h"
#include "remoting/endpointmanager.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include "base/objectlock.h"
#include "base/timer.h"
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;

static Timer::Ptr l_RetentionTimer;

REGISTER_TYPE(IcingaApplication);

#ifndef _WIN32
#	include "icinga-version.h"
#	define ICINGA_VERSION GIT_MESSAGE
#endif /* _WIN32 */

IcingaApplication::IcingaApplication(const Dictionary::Ptr& serializedUpdate)
	: Application(serializedUpdate)
{
	RegisterAttribute("cert_path", Attribute_Config, &m_CertPath);
	RegisterAttribute("ca_path", Attribute_Config, &m_CAPath);
	RegisterAttribute("node", Attribute_Config, &m_Node);
	RegisterAttribute("service", Attribute_Config, &m_Service);
	RegisterAttribute("pid_path", Attribute_Config, &m_PidPath);
	RegisterAttribute("state_path", Attribute_Config, &m_StatePath);
	RegisterAttribute("macros", Attribute_Config, &m_Macros);
}

/**
 * The entry point for the Icinga application.
 *
 * @returns An exit status.
 */
int IcingaApplication::Main(void)
{
	Log(LogDebug, "icinga", "In IcingaApplication::Main()");

	m_StartTime = Utility::GetTime();

	UpdatePidFile(GetPidPath());

	if (!GetCertificateFile().IsEmpty() && !GetCAFile().IsEmpty()) {
		/* set up SSL context */
		shared_ptr<X509> cert = GetX509Certificate(GetCertificateFile());
		String identity = GetCertificateCN(cert);
		Log(LogInformation, "icinga", "My identity: " + identity);
		EndpointManager::GetInstance()->SetIdentity(identity);

		m_SSLContext = MakeSSLContext(GetCertificateFile(), GetCertificateFile(), GetCAFile());

		EndpointManager::GetInstance()->SetSSLContext(m_SSLContext);
	}

	/* create the primary RPC listener */
	if (!GetService().IsEmpty())
		EndpointManager::GetInstance()->AddListener(GetService());

	/* restore the previous program state */
	DynamicObject::RestoreObjects(GetStatePath());

	/* periodically dump the program state */
	l_RetentionTimer = boost::make_shared<Timer>();
	l_RetentionTimer->SetInterval(300);
	l_RetentionTimer->OnTimerExpired.connect(boost::bind(&IcingaApplication::DumpProgramState, this));
	l_RetentionTimer->Start();

	RunEventLoop();

	Log(LogInformation, "icinga", "Icinga has shut down.");

	return EXIT_SUCCESS;
}

/**
 * @threadsafety Always.
 */
void IcingaApplication::OnShutdown(void)
{
	ASSERT(!OwnsLock());

	{
		ObjectLock olock(this);
		l_RetentionTimer->Stop();
	}

	DumpProgramState();
}

/**
 * @threadsafety Always.
 */
void IcingaApplication::DumpProgramState(void)
{
	DynamicObject::DumpObjects(GetStatePath());
}

IcingaApplication::Ptr IcingaApplication::GetInstance(void)
{
	return static_pointer_cast<IcingaApplication>(Application::GetInstance());
}

/**
 * @threadsafety Always.
 */
String IcingaApplication::GetCertificateFile(void) const
{
	ObjectLock olock(this);

	return m_CertPath;
}

/**
 * @threadsafety Always.
 */
String IcingaApplication::GetCAFile(void) const
{
	ObjectLock olock(this);

	return m_CAPath;
}

/**
 * @threadsafety Always.
 */
String IcingaApplication::GetNode(void) const
{
	ObjectLock olock(this);

	return m_Node;
}

/**
 * @threadsafety Always.
 */
String IcingaApplication::GetService(void) const
{
	ObjectLock olock(this);

	return m_Service;
}

/**
 * @threadsafety Always.
 */
String IcingaApplication::GetPidPath(void) const
{
	ObjectLock olock(this);

	if (m_PidPath.IsEmpty())
		return Application::GetLocalStateDir() + "/run/icinga2/icinga2.pid";
	else
		return m_PidPath;
}

/**
 * @threadsafety Always.
 */
String IcingaApplication::GetStatePath(void) const
{
	ObjectLock olock(this);

	if (m_PidPath.IsEmpty())
		return Application::GetLocalStateDir() + "/lib/icinga2/icinga2.state";
	else
		return m_PidPath;
}

/**
 * @threadsafety Always.
 */
Dictionary::Ptr IcingaApplication::GetMacros(void) const
{
	ObjectLock olock(this);

	return m_Macros;
}

/**
 * @threadsafety Always.
 */
double IcingaApplication::GetStartTime(void) const
{
	ObjectLock olock(this);

	return m_StartTime;
}

/**
 * @threadsafety Always.
 */
shared_ptr<SSL_CTX> IcingaApplication::GetSSLContext(void) const
{
	ObjectLock olock(this);

	return m_SSLContext;
}

/**
 * @threadsafety Always.
 */
Dictionary::Ptr IcingaApplication::CalculateDynamicMacros(void)
{
	Dictionary::Ptr macros = boost::make_shared<Dictionary>();

	double now = Utility::GetTime();

	macros->Set("TIMET", (long)now);
	macros->Set("LONGDATETIME", Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", now));
	macros->Set("SHORTDATETIME", Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", now));
	macros->Set("DATE", Utility::FormatDateTime("%Y-%m-%d", now));
	macros->Set("TIME", Utility::FormatDateTime("%H:%M:%S %z", now));

	return macros;
}
