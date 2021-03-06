/* Time Bar
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

#ifndef CAINTEOIRGTK_SRC_TIMEBAR_HPP
#define CAINTEOIRGTK_SRC_TIMEBAR_HPP

class TimeBar
{
public:
	TimeBar(GtkWidget *aProgress, GtkWidget *aElapsedTime, GtkWidget *aTotalTime)
		: progress(aProgress)
		, elapsedTime(aElapsedTime)
		, totalTime(aTotalTime)
	{
	}

	void update(double elapsed, double total, double completed);
private:
	GtkWidget *progress;
	GtkWidget *elapsedTime;
	GtkWidget *totalTime;
};

#endif
