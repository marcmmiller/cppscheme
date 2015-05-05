
;;
;; Scheme minimal library that assumes a scheme interpreter
;; with the following features:
;;
;; Special forms:
;;  lambda (with "rest" support)
;;  and
;;  or
;;  (define x <something>) form
;;  macroify
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
(define begin
  (lambda (stm . rest)
    (cons (cons 'lambda
                (cons '()
                      (cons stm
                            rest))) '())))
(macroify begin)

(define list
  (lambda (first . rest)
    (cons first rest)))

(define if
  (lambda (_test _t _f)
    (list 'or (list 'and _test _t) _f)))
(macroify if)

(define not
  (lambda (x) (if x #f #t)))

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

(define append 
  (lambda (l m)
    (if (null? l) m
        (cons (car l) (append (cdr l) m)))))

;;
;; need 'apply' for this to work
;; (define map
;;   (lambda (func . lst)
;;     (if (null? (car lst))
;;         '()
;;         (cons (apply func (one-map car lst))
;;               (apply map func (one-map cdr lst))))))

