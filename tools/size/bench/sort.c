void isort(int *a, unsigned n) {
  for (unsigned i = 1; i < n; i++) {
    int v = a[i];
    unsigned j = i;
    while (j > 0 && a[j - 1] > v) {
      a[j] = a[j - 1];
      j--;
    }
    a[j] = v;
  }
}

int bsearch_int(const int *a, unsigned n, int key) {
  unsigned lo = 0, hi = n;
  while (lo < hi) {
    unsigned mid = (lo + hi) / 2;
    if (a[mid] == key)
      return mid;
    if (a[mid] < key)
      lo = mid + 1;
    else
      hi = mid;
  }
  return -1;
}

int max_index(const int *a, unsigned n) {
  unsigned best = 0;
  for (unsigned i = 1; i < n; i++)
    if (a[i] > a[best])
      best = i;
  return best;
}
