/**
* Copyright (C) 2017, 2010kohtep
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#pragma once

#include <Shared\StrUtils.h>
#include <Shared\RevSpoofer.h>
#include <Shared\Encryption\CRijndael.h>
#include <Shared\Encryption\SHA.h>
#include <Windows.h>

int GenerateSC2009(void* pDest, int nSteamID)
{
	char hwid[64];

	CreateRandomString(hwid, 32);
	if (!RevSpoofer::Spoof(hwid, nSteamID))
	{
		return 0;
	}

	auto pTicket = (int*)pDest;
	auto pbTicket = (byte*)pDest;

	auto revHash = RevSpoofer::Hash(hwid);

	pTicket[0] = 'S';          // +0
	pTicket[1] = revHash;      // +4
	pTicket[2] = 'rev';        // +8
	pTicket[3] = 0;            // +12
	pTicket[4] = revHash << 1; // +16
	pTicket[5] = 0x01100001;   // +20

	/* Encrypt HWID with AESKeyRand key and save it in the ticket. */
	static const char AESKeyRand[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
	char AESHashRand[32];
	auto AESRand = CRijndael();
	AESRand.MakeKey(AESKeyRand, CRijndael::sm_chain0, 32, 32);
	AESRand.EncryptBlock(hwid, AESHashRand);
	memcpy(&pbTicket[24], AESHashRand, 32);

	/* Encrypt AESKeyRand with AESKeyRev key and save it in the ticket.
	 * AESKeyRev key is identical to the key in dproto/reunion. */
	static const char AESKeyRev[] = "_YOU_SERIOUSLY_NEED_TO_GET_LAID_";
	char AESHashRev[32];
	auto AESRev = CRijndael();
	AESRev.MakeKey(AESKeyRev, CRijndael::sm_chain0, 32, 32);
	AESRev.EncryptBlock(AESKeyRand, AESHashRev);
	memcpy(&pbTicket[56], AESHashRev, 32);

	/*  Perform HWID hashing and save hash to the ticket. */
	char SHAHash[32];
	auto sha = CSHA(CSHA::SHA256);
	sha.AddData(hwid, 32);
	sha.FinalDigest(SHAHash);
	memcpy(&pbTicket[88], SHAHash, 32);

	return 178;
}