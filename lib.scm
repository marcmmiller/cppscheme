;;
;; Minimal Scheme library that assumes a Scheme interpreter
;; with the following features:
;;
;; Special forms:
;;  lambda (with varargs support)
;;  and
;;  or
;;  define
;;  define-macro (most basic scheme macro support)
;;
;; Builtin functions:
;;  apply
;;  cons
;;  car
;;  cdr
;;  eq?
;;  null?
;;  pair?
;;
(define-macro begin
  (lambda (stm . rest)
    (cons (cons 'lambda
                (cons '()
                      (cons stm
                            rest)))
          '())))

(define (list first . rest)
  (cons first rest))

;; TODO: this should support _f as a body
(define-macro if
  (lambda (_test _t _f)
    (list 'or (list 'and _test _t) _f)))

(define (not x) (if x #f #t))

(define map
  (lambda (func list1 . more-lists)
    (define some?
      (lambda (func list)
        ;; returns #f if (func x) returns #t for
        ;; some x in the list
        (and (pair? list)
             (or (func (car list))
                 (some? func (cdr list))))))
    (define map1
      (lambda (func list)
        ;; non-variadic map.  Returns a list whose elements
        ;; the result of calling func with corresponding
        ;; elements of list
        (if (null? list)
            '()
            (cons (func (car list))
                  (map1 func (cdr list))))))
    ;; Non-variadic map implementation terminates
    ;; when any of the argument lists is empty.
    ((lambda (lists)
       (if (some? null? lists)
           '()
           (cons (apply func (map1 car lists))
                 (apply map func (map1 cdr lists)))))
     (cons list1 more-lists))))

(define (append l m)
  (if (null? l) m
      (cons (car l) (append (cdr l) m))))

(define cadr
  (lambda (p)
    (car (cdr p))))

;; parallel-binding "let"
(define-macro let
  (lambda (forms . body)
    (cons (append (cons 'lambda (list (map car forms)))
                  body)
          (map cadr forms))))

;; "cond" macro
(define-macro cond
  (lambda (f . rest)
    (define (inner clauses)
      (if (null? clauses)
          '()
          (let ((clause (car clauses))) ;; clause is like ((eq? a b) a)
            (if (eq? (car clause) 'else)
                (cadr clause)
                (append (append (list 'if (car clause)) (cdr clause))
                        (list (inner (cdr clauses))))))))
    (inner (cons f rest))))
