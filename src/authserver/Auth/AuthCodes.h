/* 
 * Project: KeenCore
 * License: GNU General Public License v2.0 or later (GPL-2.0+)
 *
 * This file is part of KeenCore.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Originally based on MaNGOS (Massive Network Game Object Server)
 * Copyright (C) 2005-2025 MaNGOS project <https://getmangos.eu>
 */

/** \file
    \ingroup authserver
*/

#ifndef _AUTHCODES
#define _AUTHCODES

/**
 * @brief
 *
 */
enum eAuthCmd
{
    CMD_AUTH_LOGON_CHALLENGE        = 0x00,
    CMD_AUTH_LOGON_PROOF            = 0x01,
    CMD_AUTH_RECONNECT_CHALLENGE    = 0x02,
    CMD_AUTH_RECONNECT_PROOF        = 0x03,
    CMD_REALM_LIST                  = 0x10,
    CMD_XFER_INITIATE               = 0x30,
    CMD_XFER_DATA                   = 0x31,
    // these opcodes no longer exist in currently supported client
    CMD_XFER_ACCEPT                 = 0x32,
    CMD_XFER_RESUME                 = 0x33,
    CMD_XFER_CANCEL                 = 0x34
};

/**
 * @brief not used by us currently
 *
 */
enum eAuthSrvCmd
{
    CMD_GRUNT_AUTH_CHALLENGE        = 0x0,
    CMD_GRUNT_AUTH_VERIFY           = 0x2,
    CMD_GRUNT_CONN_PING             = 0x10,
    CMD_GRUNT_CONN_PONG             = 0x11,
    CMD_GRUNT_HELLO                 = 0x20,
    CMD_GRUNT_PROVESESSION          = 0x21,
    CMD_GRUNT_KICK                  = 0x24,
    CMD_GRUNT_PCWARNING             = 0x29,
    CMD_GRUNT_STRINGS               = 0x41,
    CMD_GRUNT_SUNKENUPDATE          = 0x44,
    CMD_GRUNT_SUNKEN_ONLINE         = 0x46
};

/**
 * @brief
 *
 */
enum AuthResult
{
    WOW_SUCCESS                     = 0x00,
    WOW_FAIL_UNKNOWN0               = 0x01,                 ///< ? Unable to connect
    WOW_FAIL_UNKNOWN1               = 0x02,                 ///< ? Unable to connect
    WOW_FAIL_BANNED                 = 0x03,                 ///< This <game> account has been closed and is no longer available for use. Please go to <site>/banned.html for further information.
    WOW_FAIL_UNKNOWN_ACCOUNT        = 0x04,                 ///< The information you have entered is not valid. Please check the spelling of the account name and password. If you need help in retrieving a lost or stolen password, see <site> for more information
    WOW_FAIL_INCORRECT_PASSWORD     = 0x05,                 ///< The information you have entered is not valid. Please check the spelling of the account name and password. If you need help in retrieving a lost or stolen password, see <site> for more information
    // client reject next login attempts after this error, so in code used WOW_FAIL_UNKNOWN_ACCOUNT for both cases
    WOW_FAIL_ALREADY_ONLINE         = 0x06,                 ///< This account is already logged into <game>. Please check the spelling and try again.
    WOW_FAIL_NO_TIME                = 0x07,                 ///< You have used up your prepaid time for this account. Please purchase more to continue playing
    WOW_FAIL_DB_BUSY                = 0x08,                 ///< Could not log in to <game> at this time. Please try again later.
    WOW_FAIL_VERSION_INVALID        = 0x09,                 ///< Unable to validate game version. This may be caused by file corruption or interference of another program. Please visit <site> for more information and possible solutions to this issue.
    WOW_FAIL_VERSION_UPDATE         = 0x0A,                 ///< Downloading
    WOW_FAIL_INVALID_SERVER         = 0x0B,                 ///< Unable to connect
    WOW_FAIL_SUSPENDED              = 0x0C,                 ///< This <game> account has been temporarily suspended. Please go to <site>/banned.html for further information
    WOW_FAIL_FAIL_NOACCESS          = 0x0D,                 ///< Unable to connect
    WOW_SUCCESS_SURVEY              = 0x0E,                 ///< Connected.
    WOW_FAIL_PARENTCONTROL          = 0x0F,                 ///< Access to this account has been blocked by parental controls. Your settings may be changed in your account preferences at <site>
    // TBC+
    WOW_FAIL_LOCKED_ENFORCED        = 0x10,                 ///< You have applied a lock to your account. You can change your locked status by calling your account lock phone number.
    // WOTLK+
    WOW_FAIL_TRIAL_ENDED            = 0x11,                 ///< Your trial subscription has expired. Please visit <site> to upgrade your account.
    WOW_FAIL_USE_BATTLENET          = 0x12,                 ///< This account is now attached to a Battle.net account. Please login with your Battle.net account email address and password.
    WOW_FAIL_ANTI_INDULGENCE        = 0x13,                 ///< Unable to connect
    WOW_FAIL_EXPIRED                = 0x14,                 ///< Unable to connect
    WOW_FAIL_NO_GAME_ACCOUNT        = 0x15,                 ///< Unable to connect
    WOW_FAIL_CHARGEBACK             = 0x16,                 ///< This World of Warcraft account has been temporary closed due to a chargeback on its subscription. Please refer to this <site> for further information.
    WOW_FAIL_IGR_WITHOUT_BNET       = 0x17,                 ///< In order to log in to World of Warcraft using IGR time, this World of Warcraft account must first be merged with a Battle.net account. Please visit <site> to merge this account.
    WOW_FAIL_GAME_ACCOUNT_LOCKED    = 0x18,                 ///< Access to your account has been temporarily disabled.
    WOW_FAIL_UNLOCKABLE_LOCK        = 0x19,                 ///< Your account has been locked but can be unlocked.
    WOW_FAIL_CONVERSION_REQUIRED    = 0x20,                 ///< This account needs to be converted to a Battle.net account. Please [Click Here] or go to: <site> to begin conversion.
    WOW_FAIL_DISCONNECTED           = 0xFF
};

#endif
