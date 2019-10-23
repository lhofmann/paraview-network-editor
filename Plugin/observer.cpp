#include "observer.h"

Observer::Observer(const Observer& rhs) {
  for (auto observable : rhs.observables_) addObservation(observable);
}

Observer::Observer(Observer&& rhs) {
  for (auto observable : rhs.observables_) addObservation(observable);
  rhs.removeObservations();
}

Observer& Observer::operator=(const Observer& that) {
  if (this != &that) {
    removeObservations();
    for (auto observable : that.observables_) addObservation(observable);
  }
  return *this;
}

Observer& Observer::operator=(Observer&& that) {
  if (this != &that) {
    removeObservations();
    for (auto observable : that.observables_) addObservation(observable);
    that.removeObservations();
  }
  return *this;
}

Observer::~Observer() { removeObservations(); }

void Observer::removeObservation(ObservableInterface* observable) {
  if (observables_.erase(observable) > 0) {
    observable->removeObserverInternal(this);
  }
}

void Observer::removeObservations() {
  for (auto o : observables_) o->removeObserverInternal(this);
  observables_.clear();
}

void Observer::addObservation(ObservableInterface* observed) {
  std::pair<ObservableSet::iterator, bool> inserted = observables_.insert(observed);
  if (inserted.second) observed->addObserverInternal(this);
}

void Observer::addObservationInternal(ObservableInterface* observed) {
  observables_.insert(observed);
}

void Observer::removeObservationInternal(ObservableInterface* observable) {
  observables_.erase(observable);
}

void ObservableInterface::addObservationHelper(Observer* observer) {
  observer->addObservationInternal(this);
}

void ObservableInterface::removeObservationHelper(Observer* observer) {
  observer->removeObservationInternal(this);
}

