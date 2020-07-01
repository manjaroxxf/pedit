/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2013 Dominik Haumann <dhaumann@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KATE_FADE_EFFECT_H
#define KATE_FADE_EFFECT_H

#include <QObject>
#include <QPointer>

class QWidget;
class QTimeLine;
class QGraphicsOpacityEffect;
/**
 * This class provides a fade in/out effect for arbitrary QWidget%s.
 * Example:
 * \code
 * KateFadeEffect* fadeEffect = new KateFadeEffect(someWidget);
 * fadeEffect->fadeIn();
 * //...
 * fadeEffect->fadeOut();
 * \endcode
 */
class KateFadeEffect : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor.
     * By default, the widget is fully opaque (opacity = 1.0).
     */
    explicit KateFadeEffect(QWidget *widget = nullptr);

    /**
     * Check whether the hide animation started by calling fadeOut()
     * is still running. If animations are disabled, this function always
     * returns @e false.
     */
    bool isHideAnimationRunning() const;

    /**
     * Check whether the show animation started by calling fadeIn()
     * is still running. If animations are disabled, this function always
     * returns @e false.
     */
    bool isShowAnimationRunning() const;

public Q_SLOTS:
    /**
     * Call to fade out and hide the widget.
     */
    void fadeOut();

    /**
     * Call to show and fade in the widget
     */
    void fadeIn();

Q_SIGNALS:
    /**
     * This signal is emitted when the fadeOut animation is finished, started by
     * calling fadeOut(). If animations are disabled, this signal is
     * emitted immediately.
     */
    void hideAnimationFinished();

    /**
     * This signal is emitted when the fadeIn animation is finished, started by
     * calling fadeIn(). If animations are disabled, this signal is
     * emitted immediately.
     */
    void showAnimationFinished();

protected Q_SLOTS:
    /**
     * Helper to update opacity value
     */
    void opacityChanged(qreal value);

    /**
     * When the animation is finished, hide the widget if fading out.
     */
    void animationFinished();

private:
    QPointer<QWidget> m_widget;                ///< the fading widget
    QTimeLine *m_timeLine;                     ///< update time line
    QPointer<QGraphicsOpacityEffect> m_effect; ///< graphics opacity effect
};

#endif
