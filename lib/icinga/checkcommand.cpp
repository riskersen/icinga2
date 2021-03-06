/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "icinga/checkcommand.hpp"
#include "icinga/checkcommand.tcpp"
#include "base/configtype.hpp"

using namespace icinga;

REGISTER_TYPE(CheckCommand);

void CheckCommand::Execute(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
    const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	std::vector<Value> arguments;
	arguments.push_back(checkable);
	arguments.push_back(cr);
	arguments.push_back(resolvedMacros);
	arguments.push_back(useResolvedMacros);
	GetExecute()->Invoke(arguments);
}
