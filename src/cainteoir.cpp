/* Cainteoir Main Window
 *
 * Copyright (C) 2011-2012 Reece H. Dunn
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

#include <config.h>
#include <gtkmm.h>
#include <cainteoir/platform.hpp>

#include "cainteoir.hpp"
#include "gtk-compatibility.hpp"

#include <sys/stat.h>
#include <sys/types.h>

namespace rql = cainteoir::rdf::query;

static const int CHARACTERS_PER_WORD = 6;

static std::string get_user_file(const char * filename)
{
	std::string root = getenv("HOME") + std::string("/.cainteoir");
	mkdir(root.c_str(), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IROTH);

	return root + "/" + std::string(filename);
}

static double estimate_time(size_t text_length, std::tr1::shared_ptr<tts::parameter> aRate)
{
	return (double)text_length / CHARACTERS_PER_WORD / (aRate ? aRate->value() : 170) * 60.0;
}

static void display_error_message(GtkWindow *window, const char *title, const char *text, const char *secondary_text)
{
	GtkWidget *dialog = gtk_message_dialog_new(window,
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_CLOSE,
		"%s", text);
	gtk_window_set_title(GTK_WINDOW(dialog), title);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", secondary_text);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void create_recent_filter(GtkObjectRef<Gtk::RecentFilter> &filter, const rdf::graph & aMetadata)
{
	rql::results formats = rql::select(aMetadata,
		rql::both(rql::matches(rql::predicate, rdf::rdf("type")),
		          rql::matches(rql::object, rdf::tts("DocumentFormat"))));


	for(auto format = formats.begin(), last = formats.end(); format != last; ++format)
	{
		const rdf::uri * uri = rql::subject(*format);

		rql::results mimetypes = rql::select(aMetadata,
			rql::both(rql::matches(rql::predicate, rdf::tts("mimetype")),
			          rql::matches(rql::subject, *uri)));

		for(auto mimetype = mimetypes.begin(), last = mimetypes.end(); mimetype != last; ++mimetype)
			filter->add_mime_type(rql::value(*mimetype));
	}
}

static GtkWidget *create_padded_container(GtkWidget *child, int padding_width, int padding_height)
{
	GtkWidget *left_right = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(left_right), child, TRUE, TRUE, padding_width);

	GtkWidget *top_bottom = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start(GTK_BOX(top_bottom), left_right, TRUE, TRUE, padding_height);

	return top_bottom;
}

struct NavButtonData
{
	GtkWidget *view;
	int page;
};

static void on_navbutton_clicked(GtkTreeViewColumn *column, void *data)
{
	NavButtonData *navbutton = (NavButtonData *)data;
	gtk_notebook_set_current_page(GTK_NOTEBOOK(navbutton->view), navbutton->page);
}

static GtkWidget *create_navbutton(const char *label, GtkWidget *view, int page)
{
	NavButtonData *data = g_slice_new(NavButtonData);
	data->view = view;
	data->page = page;

	GtkWidget *navbutton = gtk_button_new_with_label(label);
	g_signal_connect(navbutton, "clicked", G_CALLBACK(on_navbutton_clicked), data);
	return navbutton;
}

Cainteoir::Cainteoir(const char *filename)
	: doc_metadata(languages, _("<b>Document</b>"), 5)
	, voice_metadata(languages, _("<b>Voice</b>"), 2)
	, engine_metadata(languages, _("<b>Engine</b>"), 2)
	, languages("en")
	, readButton(Gtk::Stock::MEDIA_PLAY)
	, stopButton(Gtk::Stock::MEDIA_STOP)
	, recordButton(Gtk::Stock::MEDIA_RECORD)
	, openButton(Gtk::Stock::OPEN)
	, settings(get_user_file("settings.dat"))
{
	voiceSelection = std::shared_ptr<VoiceSelectionView>(new VoiceSelectionView(settings, doc.tts, doc.m_metadata, languages));
	voiceSelection->signal_on_voice_change().connect(sigc::mem_fun(*this, &Cainteoir::switch_voice));

	set_title(_("Cainteoir Text-to-Speech"));
	set_size_request(500, 300);

	gtk_window_set_hide_titlebar_when_maximized(GTK_WINDOW(gobj()), TRUE);

	resize(settings("window.width",  700).as<int>(), settings("window.height", 445).as<int>());
	move(settings("window.left", 0).as<int>(), settings("window.top",  0).as<int>());
	if (settings("window.maximized", "false").as<std::string>() == "true")
		maximize();

	signal_window_state_event().connect(sigc::mem_fun(*this, &Cainteoir::on_window_state_changed));
	signal_delete_event().connect(sigc::mem_fun(*this, &Cainteoir::on_delete));

	create_recent_filter(recentFilter, doc.m_metadata);
	recentManager = Gtk::RecentManager::get_default();
	recentAction = Gtk::Action::create("FileRecentFiles", _("_Recent Documents"));

	readButton.signal_clicked().connect(sigc::mem_fun(*this, &Cainteoir::on_read));
	stopButton.signal_clicked().connect(sigc::mem_fun(*this, &Cainteoir::on_stop));
	recordButton.signal_clicked().connect(sigc::mem_fun(*this, &Cainteoir::on_record));

	readButton.set_border_width(0);
	stopButton.set_border_width(0);
	recordButton.set_border_width(0);
	openButton.set_border_width(0);

	openButton.signal_clicked().connect(sigc::mem_fun(*this, &Cainteoir::on_open_document));
	openButton.set_menu(*create_file_chooser_menu());

	GtkWidget *navbar = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_name(navbar, "navbar");

	GtkWidget *topbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_size_request(topbar, 0, 50);
	gtk_box_pack_start(GTK_BOX(topbar), navbar, FALSE, FALSE, 10);
	gtk_box_pack_start(GTK_BOX(topbar), gtk_label_new(""), TRUE, TRUE, 0); // stretchy
	gtk_box_pack_start(GTK_BOX(topbar), GTK_WIDGET(openButton.gobj()), FALSE, FALSE, 0);

	GtkWidget *bottombar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_size_request(bottombar, 0, 50);
	gtk_box_pack_start(GTK_BOX(bottombar), GTK_WIDGET(readButton.gobj()), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(bottombar), GTK_WIDGET(stopButton.gobj()), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(bottombar), GTK_WIDGET(recordButton.gobj()), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(bottombar), timebar, TRUE, TRUE, 0);

	doc_metadata.create_entry(rdf::dc("title"), _("Title"), 0);
	doc_metadata.create_entry(rdf::dc("creator"), _("Author"), 1);
	doc_metadata.create_entry(rdf::dc("publisher"), _("Publisher"), 2);
	doc_metadata.create_entry(rdf::dc("description"), _("Description"), 3);
	doc_metadata.create_entry(rdf::dc("language"), _("Language"), 4);
	doc_metadata.create_entry(rdf::tts("length"), _("Length"), 5);

	voice_metadata.create_entry(rdf::tts("name"), _("Name"), 0);
	voice_metadata.create_entry(rdf::dc("language"), _("Language"), 1);

	engine_metadata.create_entry(rdf::tts("name"), _("Name"), 0);
	engine_metadata.create_entry(rdf::tts("version"), _("Version"), 1);

	metadata_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start(GTK_BOX(metadata_view), doc_metadata, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(metadata_view), voice_metadata, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(metadata_view), engine_metadata, FALSE, FALSE, 0);

	GtkWidget *toc = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_size_request(toc, 300, 0);
	gtk_box_pack_start(GTK_BOX(toc), doc.toc, TRUE, TRUE, 0);

	GtkWidget *pane = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 30);
	gtk_box_pack_start(GTK_BOX(pane), toc, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pane), metadata_view, TRUE, TRUE, 0);

	view = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(view), FALSE);

	int doc_page = gtk_notebook_append_page(GTK_NOTEBOOK(view), create_padded_container(pane, 5, 5), NULL);
	gtk_box_pack_start(GTK_BOX(navbar), create_navbutton(_("Document"), view, doc_page), FALSE, FALSE, 0);

	int voice_page = gtk_notebook_append_page(GTK_NOTEBOOK(view), GTK_WIDGET(voiceSelection->gobj()),  NULL);
	gtk_box_pack_start(GTK_BOX(navbar), create_navbutton(_("Voice"), view, voice_page), FALSE, FALSE, 0);

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(gobj()), box);
	gtk_box_pack_start(GTK_BOX(box), topbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), view, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), bottombar, FALSE, FALSE, 0);

	timebar.update(0.0, estimate_time(doc.m_doc->text_length(), doc.tts.parameter(tts::parameter::rate)), 0.0);

	show_all_children();

	readButton.set_sensitive(false);
	stopButton.set_visible(false);

	load_document(filename ? std::string(filename) : settings("document.filename").as<std::string>());
	switch_voice(doc.tts.voice());
}

bool Cainteoir::on_window_state_changed(GdkEventWindowState *event)
{
	settings("window.maximized") = (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) ? "true" : "false";
	settings.save();
	return true;
}

bool Cainteoir::on_delete(GdkEventAny *event)
{
	on_quit();
	return true;
}

void Cainteoir::on_open_document()
{
	Gtk::FileChooserDialog dialog(_("Open Document"), Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.set_transient_for(*this);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
	dialog.set_filename(settings("document.filename").as<std::string>());

	rql::results formats = rql::select(doc.m_metadata,
		rql::both(rql::matches(rql::predicate, rdf::rdf("type")),
		          rql::matches(rql::object, rdf::tts("DocumentFormat"))));

	std::string default_mimetype = settings("document.mimetype", "text/plain").as<std::string>();

	for(auto format = formats.begin(), last = formats.end(); format != last; ++format)
	{
		rql::results data = rql::select(doc.m_metadata, rql::matches(rql::subject, rql::subject(*format)));

		GtkObjectRef<Gtk::FileFilter> filter;
		filter->set_name(rql::select_value<std::string>(data, rql::matches(rql::predicate, rdf::dc("title"))));

		rql::results mimetypes = rql::select(data, rql::matches(rql::predicate, rdf::tts("mimetype")));

		bool active_filter = false;
		for(auto item = mimetypes.begin(), last = mimetypes.end(); item != last; ++item)
		{
			const std::string & mimetype = rql::value(*item);
			filter->add_mime_type(mimetype);
			if (default_mimetype == mimetype)
				active_filter = true;
		}

 		dialog.add_filter(filter);
		if (active_filter)
			dialog.set_filter(filter);
	}

	if (dialog.run() == Gtk::RESPONSE_OK)
		load_document(dialog.get_filename());
}

void Cainteoir::on_recent_files_dialog()
{
	GtkWidget *dialog = gtk_recent_chooser_dialog_new(_("Recent Documents"), GTK_WINDOW(gobj()),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN,   GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_window_resize(GTK_WINDOW(dialog), 500, 200);
	gtk_recent_chooser_set_filter(GTK_RECENT_CHOOSER(dialog), GTK_RECENT_FILTER(recentFilter->gobj()));
	gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(dialog), GTK_RECENT_SORT_MRU);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GtkRecentInfo *info = gtk_recent_chooser_get_current_item(GTK_RECENT_CHOOSER(dialog));
		gchar *filename = gtk_recent_info_get_uri_display(info);
		if (filename)
		{
			load_document(filename);
			g_free(filename);
		}
		gtk_recent_info_unref(info);
	}

	gtk_widget_destroy(dialog);
}

void Cainteoir::on_recent_file(Gtk::RecentChooserMenu * recent)
{
	GtkRecentInfo *info = gtk_recent_chooser_get_current_item(GTK_RECENT_CHOOSER(recent->gobj()));
	gchar *filename = gtk_recent_info_get_uri_display(info);
	if (filename)
	{
		load_document(filename);
		g_free(filename);
	}
	gtk_recent_info_unref(info);
}

void Cainteoir::on_quit()
{
	if (speech)
		speech->stop();

	if (settings("window.maximized", "false").as<std::string>() == "false")
	{
		int width = 0;
		int height = 0;
		int top = 0;
		int left = 0;

		get_position(left, top);
		get_size(width, height);

		settings("window.width")  = width;
		settings("window.height") = height;
		settings("window.top")    = top;
		settings("window.left")   = left;
	}
	settings.save();

	hide();
}

void Cainteoir::on_read()
{
	out = cainteoir::open_audio_device(NULL, "pulse", 0.3, doc.m_metadata, *doc.subject, doc.tts.voice());
	on_speak(_("reading"));
}

void Cainteoir::on_record()
{
	// TODO: Generate a default name from the file metadata ($(recording.basepath)/author - title.ogg)

	Gtk::FileChooserDialog dialog(_("Record Document"), Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.set_transient_for(*this);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::MEDIA_RECORD, Gtk::RESPONSE_OK);
	dialog.set_filename(settings("recording.filename").as<std::string>());

	rql::results formats = rql::select(doc.m_metadata,
		rql::both(rql::matches(rql::predicate, rdf::rdf("type")),
		          rql::matches(rql::object, rdf::tts("AudioFormat"))));

	std::string default_mimetype = settings("recording.mimetype", "audio/x-vorbis+ogg").as<std::string>();

	for (auto format = formats.begin(), last = formats.end(); format != last; ++format)
	{
		rql::results data = rql::select(doc.m_metadata, rql::matches(rql::subject, rql::subject(*format)));

		GtkObjectRef<Gtk::FileFilter> filter;
		filter->set_name(rql::select_value<std::string>(data, rql::matches(rql::predicate, rdf::dc("title"))));

		rql::results mimetypes = rql::select(data, rql::matches(rql::predicate, rdf::tts("mimetype")));

		bool active_filter = false;
		for(auto item = mimetypes.begin(), last = mimetypes.end(); item != last; ++item)
		{
			const std::string & mimetype = rql::value(*item);
			filter->add_mime_type(mimetype);
			if (default_mimetype == mimetype)
				active_filter = true;
		}

 		dialog.add_filter(filter);
		if (active_filter)
			dialog.set_filter(filter);
	}

	if (dialog.run() != Gtk::RESPONSE_OK)
		return;

	std::string filename = dialog.get_filename();

	std::string::size_type extpos = filename.rfind('.');
	if (extpos != std::string::npos)
	{
		std::string ext = '*' + filename.substr(extpos);

		rql::results filetypes = rql::select(doc.m_metadata,
			rql::both(rql::matches(rql::predicate, rdf::tts("extension")),
			          rql::matches(rql::object, rdf::literal(ext))));

		if (filetypes.size() == 1)
		{
			const rdf::uri *uri = rql::subject(filetypes.front());
			if (uri)
			{
				std::string type = rql::select_value<std::string>(doc.m_metadata,
					rql::both(rql::matches(rql::subject, *uri),
					          rql::matches(rql::predicate, rdf::tts("name"))));

				std::string mimetype = rql::select_value<std::string>(doc.m_metadata,
					rql::both(rql::matches(rql::subject, *uri),
					          rql::matches(rql::predicate, rdf::tts("mimetype"))));

				settings("recording.filename") = filename;
				settings("recording.mimetype") = mimetype;
				settings.save();

				out = cainteoir::create_audio_file(filename.c_str(), type.c_str(), 0.3, doc.m_metadata, *doc.subject, doc.tts.voice());
				on_speak(_("recording"));
				return;
			}
		}
	}

	display_error_message(GTK_WINDOW(gobj()),
		_("Record Document"),
		_("Unable to record the document"),
		_("Unsupported file type."));
}

void Cainteoir::on_speak(const char * status)
{
	cainteoir::document::range_type selection = doc.selection();
	speech = doc.tts.speak(doc.m_doc, out, selection.first, selection.second);

	readButton.set_visible(false);
	stopButton.set_visible(true);
	recordButton.set_sensitive(false);

	openButton.set_sensitive(false);
	recentAction->set_sensitive(false);

	Glib::signal_timeout().connect(sigc::mem_fun(*this, &Cainteoir::on_speaking), 100);
}

void Cainteoir::on_stop()
{
	speech->stop();
}

bool Cainteoir::on_speaking()
{
	if (speech->is_speaking())
	{
		timebar.update(speech->elapsedTime(), speech->totalTime(), speech->completed());
		return true;
	}

	std::string error = speech->error_message();
	if (!error.empty())
	{
		display_error_message(GTK_WINDOW(gobj()),
			_("Cainteoir Text-to-Speech"),
			_("Error speaking the document"),
			error.c_str());
	}

	speech.reset();
	out.reset();

	timebar.update(0.0, estimate_time(doc.m_doc->text_length(), doc.tts.parameter(tts::parameter::rate)), 0.0);

	readButton.set_visible(true);
	stopButton.set_visible(false);
	recordButton.set_sensitive(true);

	openButton.set_sensitive(true);
	recentAction->set_sensitive(true);

	return false;
}

bool Cainteoir::load_document(std::string filename)
{
	if (speech || filename.empty()) return false;

	readButton.set_sensitive(false);

	doc.clear();
	doc_metadata.clear();

	try
	{
		if (filename.find("file://") == 0)
			filename.erase(0, 7);

		if (cainteoir::parseDocument(filename.c_str(), doc, doc.m_metadata))
		{
			doc.subject = std::tr1::shared_ptr<const rdf::uri>(new rdf::uri(filename, std::string()));
			recentManager->add_item("file://" + filename);

			rql::results data = rql::select(doc.m_metadata, rql::matches(rql::subject, *doc.subject));
			std::string mimetype = rql::select_value<std::string>(data, rql::matches(rql::predicate, rdf::tts("mimetype")));
			std::string title    = rql::select_value<std::string>(data, rql::matches(rql::predicate, rdf::dc("title")));

			if (doc.toc.empty())
				gtk_widget_hide(doc.toc);
			else
				gtk_widget_show(doc.toc);

			settings("document.filename") = filename;
			if (!mimetype.empty())
				settings("document.mimetype") = mimetype;
			settings.save();

			doc_metadata.add_metadata(doc.m_metadata, *doc.subject, rdf::dc("title"));
			doc_metadata.add_metadata(doc.m_metadata, *doc.subject, rdf::dc("creator"));
			doc_metadata.add_metadata(doc.m_metadata, *doc.subject, rdf::dc("publisher"));
			doc_metadata.add_metadata(doc.m_metadata, *doc.subject, rdf::dc("description"));
			doc_metadata.add_metadata(doc.m_metadata, *doc.subject, rdf::dc("language"));

			std::ostringstream length;
			length << (doc.m_doc->text_length() / CHARACTERS_PER_WORD) << _(" words (approx.)");

			doc_metadata.add_metadata(rdf::tts("length"), length.str().c_str());

			timebar.update(0.0, estimate_time(doc.m_doc->text_length(), doc.tts.parameter(tts::parameter::rate)), 0.0);

			std::string lang = rql::select_value<std::string>(doc.m_metadata,
				rql::both(rql::matches(rql::subject, *doc.subject),
				          rql::matches(rql::predicate, rdf::dc("language"))));
			if (lang.empty())
				lang = "en";

			switch_voice_by_language(lang);

			readButton.set_sensitive(true);
			return true;
		}
	}
	catch (const std::exception & e)
	{
		display_error_message(GTK_WINDOW(gobj()),
			_("Open Document"),
			_("Unable to open the document"),
			e.what());
	}

	return false;
}

Gtk::Menu *Cainteoir::create_file_chooser_menu()
{
	Gtk::RecentChooserMenu *recent = Gtk::manage(new Gtk::RecentChooserMenu(recentManager));

	recent->signal_item_activated().connect(sigc::bind(sigc::mem_fun(*this, &Cainteoir::on_recent_file), recent));
	recent->set_show_numbers(true);
	recent->set_sort_type(Gtk::RECENT_SORT_MRU);
	recent->set_filter(recentFilter);
	recent->set_limit(6);

	Gtk::MenuItem *separator = Gtk::manage(new Gtk::SeparatorMenuItem());
	recent->append(*separator);
	separator->show();

	Gtk::MenuItem *more = Gtk::manage(new Gtk::MenuItem(_("_More Documents..."), true));
	more->signal_activate().connect(sigc::mem_fun(*this, &Cainteoir::on_recent_files_dialog));
	recent->append(*more);
	more->show();

	return recent;
}

bool Cainteoir::switch_voice(const rdf::uri &voice)
{
	voice_metadata.add_metadata(doc.m_metadata, voice, rdf::tts("name"));
	voice_metadata.add_metadata(doc.m_metadata, voice, rdf::dc("language"));

	foreach_iter(statement, rql::select(doc.m_metadata, rql::matches(rql::subject, voice)))
	{
		if (rql::predicate(*statement) == rdf::tts("voiceOf"))
		{
			const rdf::uri *engine = rql::object(*statement);
			if (engine)
			{
				engine_metadata.add_metadata(doc.m_metadata, *engine, rdf::tts("name"));
				engine_metadata.add_metadata(doc.m_metadata, *engine, rdf::tts("version"));
			}
		}
	}

	if (doc.tts.select_voice(doc.m_metadata, voice))
	{
		voiceSelection->show(doc.tts.voice());
		return true;
	}

	return false;
}

bool Cainteoir::switch_voice_by_language(const std::string &language)
{
	std::string language2 = language.substr(0, language.find('-'));

	// Does the current voice support this language? ...

	std::string current = rql::select_value<std::string>(doc.m_metadata,
		rql::both(rql::matches(rql::subject, doc.tts.voice()),
		          rql::matches(rql::predicate, rdf::dc("language"))));

	if (current == language || current == language2)
		return true;

	current = current.substr(0, current.find('-'));
	if (current == language || current == language2)
		return true;

	// The current voice does not support this language, so search the available voices ...

	rql::results voicelist = rql::select(doc.m_metadata,
		rql::both(rql::matches(rql::predicate, rdf::rdf("type")),
		          rql::matches(rql::object, rdf::tts("Voice"))));

	foreach_iter(voice, voicelist)
	{
		const rdf::uri *uri = rql::subject(*voice);
		if (uri)
		{
			std::string lang = rql::select_value<std::string>(doc.m_metadata,
				rql::both(rql::matches(rql::subject, *uri),
				          rql::matches(rql::predicate, rdf::dc("language"))));

			if ((lang == language || lang == language2) && switch_voice(*uri))
				return true;

			// Handle specific voices against generic document languages, e.g.
			// eSpeak has 'pt-pt' and 'pt-br' voices, but no 'pt' voice and the
			// Project Guttenberg ebooks report portuguese as 'pt'.

			lang = lang.substr(0, lang.find('-'));
			if ((lang == language || lang == language2) && switch_voice(*uri))
				return true;
		}
	}
	return false;
}
