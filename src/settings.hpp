/* Application Settings
 *
 * Copyright (C) 2011 Reece H. Dunn
 *
 * This file is part of cainteoir-gtk.
 *
 * cainteoir-gtk is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cainteoir-gtk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cainteoir-gtk.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CAINTEOIRGTK_SRC_SETTINGS_HPP
#define CAINTEOIRGTK_SRC_SETTINGS_HPP

#include <map>
#include <cainteoir/metadata.hpp>

namespace rdf = cainteoir::rdf;

class application_settings
{
public:
	application_settings(const std::string &aFilename);

	void save();

	rdf::literal & operator()(const std::string & name, const rdf::literal & default_value = rdf::literal());
private:
	std::map<std::string, rdf::literal> values;
	std::string filename;
};

#endif
