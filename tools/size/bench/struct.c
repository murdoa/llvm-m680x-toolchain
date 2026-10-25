struct node {
  struct node *next;
  int key;
  char tag;
};

struct node *find(struct node *n, int key) {
  for (; n; n = n->next)
    if (n->key == key)
      return n;
  return 0;
}

struct node *list_reverse(struct node *n) {
  struct node *r = 0;
  while (n) {
    struct node *next = n->next;
    n->next = r;
    r = n;
    n = next;
  }
  return r;
}

unsigned count_tag(const struct node *n, char tag) {
  unsigned c = 0;
  for (; n; n = n->next)
    c += n->tag == tag;
  return c;
}

struct ring {
  unsigned char buf[32];
  unsigned char head, tail;
};

int ring_put(struct ring *r, unsigned char c) {
  unsigned char next = (r->head + 1) & 31;
  if (next == r->tail)
    return 0;
  r->buf[r->head] = c;
  r->head = next;
  return 1;
}

int ring_get(struct ring *r) {
  if (r->head == r->tail)
    return -1;
  unsigned char c = r->buf[r->tail];
  r->tail = (r->tail + 1) & 31;
  return c;
}
