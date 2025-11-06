// Optimiert: Vorberechneter Binomial-Cache für häufig verwendete Werte
static const int binomialCache[11][11] = {
  {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  // n=0
  {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},  // n=1
  {1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},  // n=2
  {1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0},  // n=3
  {1, 4, 6, 4, 1, 0, 0, 0, 0, 0, 0},  // n=4
  {1, 5, 10, 10, 5, 1, 0, 0, 0, 0, 0},  // n=5
  {1, 6, 15, 20, 15, 6, 1, 0, 0, 0, 0},  // n=6
  {1, 7, 21, 35, 35, 21, 7, 1, 0, 0, 0},  // n=7
  {1, 8, 28, 56, 70, 56, 28, 8, 1, 0, 0},  // n=8
  {1, 9, 36, 84, 126, 126, 84, 36, 9, 1, 0},  // n=9
  {1, 10, 45, 120, 210, 252, 210, 120, 45, 10, 1}  // n=10
};

// Optimiert: Schnelle pow() für kleine ganzzahlige Exponenten
inline float fastPow(float base, int exp) {
  if(exp == 0) return 1.0f;
  if(exp == 1) return base;

  float result = 1.0f;
  for(int i = 0; i < exp; i++) {
    result *= base;
  }
  return result;
}

Vector2 GetPointOnBezierCurve(Vector2* points, int numPoints, float t) {
  Vector2 pos;

  // Optimiert: fastPow() statt pow() für bessere Performance
  float oneMinusT = 1.0f - t;
  for (int i = 0; i < numPoints; i++) {
    float b = binomialCoefficient(numPoints - 1, i) * fastPow(oneMinusT, numPoints - 1 - i) * fastPow(t, i);
    pos.x += b * points[i].x;
    pos.y += b * points[i].y;
  }

  return pos;
}


Vector3 GetPointOnBezierCurve(Vector3* points, int numPoints, float t) {
  Vector3 pos;

  // Optimiert: fastPow() statt pow() für bessere Performance
  float oneMinusT = 1.0f - t;
  for (int i = 0; i < numPoints; i++) {
    float b = binomialCoefficient(numPoints - 1, i) * fastPow(oneMinusT, numPoints - 1 - i) * fastPow(t, i);
    pos.x += b * points[i].x;
    pos.y += b * points[i].y;
    pos.z += b * points[i].z;
  }

  return pos;
}

int binomialCoefficient(int n, int k) {
  // Optimiert: Cache-Lookup für häufig verwendete Werte (n <= 10)
  if(n <= 10 && k <= 10 && k <= n) {
    return binomialCache[n][k];
  }

  // Fallback für größere Werte (sollte selten verwendet werden)
  int result = 1;
  for (int i = 1; i <= k; i++) {
    result *= (n - (k - i));
    result /= i;
  }
  return result;
}