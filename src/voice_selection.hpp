/* Voice Selection View
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

#ifndef CAINTEOIRGTK_SRC_VOICESELECTION_HPP
#define CAINTEOIRGTK_SRC_VOICESELECTION_HPP

#include <cainteoir/engines.hpp>
#include <cainteoir/languages.hpp>

namespace rdf = cainteoir::rdf;
namespace rql = cainteoir::rdf::query;
namespace tts = cainteoir::tts;

class VoiceList
{
public:
	VoiceList(rdf::graph &aMetadata, cainteoir::languages &languages);

	operator GtkWidget *() { return layout; }

	void add_voice(rdf::graph &aMetadata, rql::results &voice, cainteoir::languages &languages);
private:
	GtkWidget *layout;
	GtkTreeStore *store;
	GtkWidget *tree;
};

struct VoiceParameter
{
	tts::parameter::type type;
	Gtk::Label  *label;
	Gtk::HScale *param;
	Gtk::Label  *units;
};

class VoiceSelectionView : public Gtk::VBox
{
public:
	VoiceSelectionView(tts::engines &aEngines, rdf::graph &aMetadata, cainteoir::languages &languages);

	void show();
protected:
	void apply_settings();
private:
	void create_entry(tts::parameter::type, int row);

	Gtk::Label voices_header;
	VoiceList voices;

	std::list<VoiceParameter> parameters;
	tts::engines *mEngines;

	Gtk::Label header;
	Gtk::Table parameterView;

	Gtk::HButtonBox buttons;
	Gtk::Button apply;
};

#endif
