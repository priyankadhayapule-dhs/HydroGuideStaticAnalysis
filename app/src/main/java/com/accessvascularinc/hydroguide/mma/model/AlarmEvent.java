package com.accessvascularinc.hydroguide.mma.model;

import java.util.Objects;

public class AlarmEvent implements Comparable<AlarmEvent> {
  private AlarmType alarm = null;
  private int priority = 0;
  private String compositeAlerts = "";

  public AlarmEvent() {

  }

  public AlarmEvent(final AlarmType alarm, final int priority) {
    this.alarm = alarm;
    this.priority = priority;
  }

  public AlarmType getAlarmType() {
    return alarm;
  }

  public int getPriority() {
    return priority;
  }

  public String getCompositeAlerts() {
    return compositeAlerts;
  }

  public void setCompositeAlerts(final String compositeAlerts) {
    this.compositeAlerts = compositeAlerts;
  }

  // Comparators
  @Override
  public int compareTo(final AlarmEvent rhs) {
    final int priorityComparison = Integer.compare(this.priority, rhs.priority);

    if (priorityComparison == 0) {
      return Integer.compare(this.alarm.getNumVal(), rhs.alarm.getNumVal());
    }

    return priorityComparison;
  }

  @Override
  public boolean equals(final Object rhs) {
    if (this == rhs) {
      return true;
    }
    if (rhs == null || getClass() != rhs.getClass()) {
      return false;
    }
    final AlarmEvent other = (AlarmEvent) rhs;
    return (this.priority == other.priority && this.alarm == other.alarm);
  }

  @Override
  public int hashCode() {
    return Objects.hash(alarm, priority);
  }

  @Override
  public String toString() {
    return "AlarmEvent {" + "alarmType='" + alarm + ", priority=" + priority + '}';
  }
}
