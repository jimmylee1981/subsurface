#include "DiveObjectHelper.h"

#include <QDateTime>
#include <QTextDocument>

#include "../qthelper.h"
#include "../helpers.h"

static QString EMPTY_DIVE_STRING = QStringLiteral("--");


static QString getFormattedWeight(struct dive *dive, unsigned int idx)
{
        weightsystem_t *weight = &dive->weightsystem[idx];
        if (!weight->description)
                return QString(EMPTY_DIVE_STRING);
        QString fmt = QString(weight->description);
        fmt += ", " + get_weight_string(weight->weight, true);
        return fmt;
}

static QString getFormattedCylinder(struct dive *dive, unsigned int idx)
{
        cylinder_t *cyl = &dive->cylinder[idx];
        const char *desc = cyl->type.description;
        if (!desc && idx > 0)
                return QString(EMPTY_DIVE_STRING);
        QString fmt = desc ? QString(desc) : QObject::tr("unknown");
        fmt += ", " + get_volume_string(cyl->type.size, true, 0);
        fmt += ", " + get_pressure_string(cyl->type.workingpressure, true);
        fmt += ", " + get_pressure_string(cyl->start, false) + " - " + get_pressure_string(cyl->end, true);
        fmt += ", " + get_gas_string(cyl->gasmix);
        return fmt;
}

DiveObjectHelper::DiveObjectHelper(struct dive *d) :
	m_number(d->number),
	m_id(d->id),
	m_rating(d->rating),
	m_timestamp(d->when),
	m_location(get_dive_location(d) ? QString::fromUtf8(get_dive_location(d)) : EMPTY_DIVE_STRING),
	m_duration(get_dive_duration_string(d->duration.seconds, QObject::tr("h:"), QObject::tr("min"))),
	m_depth(get_depth_string(d->dc.maxdepth.mm, true, true)),
	m_divemaster(d->divemaster ? d->divemaster : EMPTY_DIVE_STRING),
	m_buddy(d->buddy ? d->buddy : EMPTY_DIVE_STRING),
	m_airTemp(get_temperature_string(d->airtemp, true)),
	m_waterTemp(get_temperature_string(d->watertemp, true)),
	m_suit(d->suit ? d->suit : EMPTY_DIVE_STRING),
	m_trip(d->divetrip ? d->divetrip->location : EMPTY_DIVE_STRING),
	m_maxcns(d->maxcns),
	m_otu(d->otu),
	m_dive(d)
{
	struct dive_site *ds = get_dive_site_by_uuid(d->dive_site_uuid);
	if (ds)
		m_gps = QString("%1,%2").arg(ds->latitude.udeg / 1000000.0).arg(ds->longitude.udeg / 1000000.0);

	if (m_airTemp.isEmpty()) {
		m_airTemp = EMPTY_DIVE_STRING;
	}
	if (m_waterTemp.isEmpty()) {
		m_waterTemp = EMPTY_DIVE_STRING;
	}

	m_notes = QString::fromUtf8(d->notes);
	if (m_notes.isEmpty()) {
		m_notes = EMPTY_DIVE_STRING;
		return;
	}
	if (same_string(d->dc.model, "planned dive")) {
		QTextDocument notes;
		QString notesFormatted = m_notes;
	#define _NOTES_BR "&#92n"
		notesFormatted = notesFormatted.replace("<thead>", "<thead>" _NOTES_BR);
		notesFormatted = notesFormatted.replace("<br>", "<br>" _NOTES_BR);
		notesFormatted = notesFormatted.replace("<tr>", "<tr>" _NOTES_BR);
		notesFormatted = notesFormatted.replace("</tr>", "</tr>" _NOTES_BR);
		notes.setHtml(notesFormatted);
		m_notes = notes.toPlainText();
		m_notes.replace(_NOTES_BR, "<br>");
	#undef _NOTES_BR
	} else {
		m_notes.replace("\n", "<br>");
	}


	char buffer[256];
	taglist_get_tagstring(d->tag_list, buffer, 256);
	m_tags = QString(buffer);


	int added = 0;
	QString gas, gases;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (!is_cylinder_used(d, i))
			continue;
		gas = d->cylinder[i].type.description;
		gas += QString(!gas.isEmpty() ? " " : "") + gasname(&d->cylinder[i].gasmix);
		// if has a description and if such gas is not already present
		if (!gas.isEmpty() && gases.indexOf(gas) == -1) {
			if (added > 0)
				gases += QString(" / ");
			gases += gas;
			added++;
		}
	}
	m_gas = gases;

	if (d->sac) {
		const char *unit;
		int decimal;
		double value = get_volume_units(d->sac, &decimal, &unit);
		m_sac = QString::number(value, 'f', decimal).append(unit);
	}

	for (int i = 0; i < MAX_CYLINDERS; i++)
		m_cylinders << getFormattedCylinder(d, i);

	for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++)
		m_weights << getFormattedWeight(d, i);

	QDateTime localTime = QDateTime::fromTime_t(d->when - gettimezoneoffset(d->when));
	localTime.setTimeSpec(Qt::UTC);
	m_date = localTime.date().toString(prefs.date_format);
	m_time = localTime.time().toString(prefs.time_format);
}

DiveObjectHelper::~DiveObjectHelper()
{
}

int DiveObjectHelper::number() const
{
	return m_number;
}

int DiveObjectHelper::id() const
{
	return m_id;
}

QString DiveObjectHelper::date() const
{
	return m_date;
}

timestamp_t DiveObjectHelper::timestamp() const
{
	return m_timestamp;
}

QString DiveObjectHelper::time() const
{
	return m_time;
}

QString DiveObjectHelper::location() const
{
	return m_location;
}

QString DiveObjectHelper::gps() const
{
	return m_gps;
}
QString DiveObjectHelper::duration() const
{
	return m_duration;
}

QString DiveObjectHelper::depth() const
{
	return m_depth;
}

QString DiveObjectHelper::divemaster() const
{
	return m_divemaster;
}

QString DiveObjectHelper::buddy() const
{
	return m_buddy;
}

QString DiveObjectHelper::airTemp() const
{
	return m_airTemp;
}

QString DiveObjectHelper::waterTemp() const
{
	return m_waterTemp;
}

QString DiveObjectHelper::notes() const
{
	return m_notes;
}

QString DiveObjectHelper::tags() const
{
	return m_tags;
}

QString DiveObjectHelper::gas() const
{
	return m_gas;
}

QString DiveObjectHelper::sac() const
{
	return m_sac;
}

QStringList DiveObjectHelper::weights() const
{
return m_weights;
}

QString DiveObjectHelper::weight(int idx) const
{
	if (idx < 0 || idx > m_weights.size() - 1)
		return QString(EMPTY_DIVE_STRING);
	return m_weights.at(idx);
}

QString DiveObjectHelper::suit() const
{
	return m_suit;
}

QStringList DiveObjectHelper::cylinders() const
{
	return m_cylinders;
}

QString DiveObjectHelper::cylinder(int idx) const
{
	if (idx < 0 || idx > m_cylinders.size() - 1)
		return QString(EMPTY_DIVE_STRING);
	return m_cylinders.at(idx);
}

QString DiveObjectHelper::trip() const
{
	return m_trip;
}

QString DiveObjectHelper::maxcns() const
{
	return m_maxcns;
}

QString DiveObjectHelper::otu() const
{
	return m_otu;
}

int DiveObjectHelper::rating() const
{
	return m_rating;
}