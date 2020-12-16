var highlighted = null;
var selected = null;

function highlight(elementID) {
    if (highlighted) {
        highlighted.style.background = "";
    }
    highlighted = document.getElementById(elementID);
    if (highlighted) {
        highlighted.style.background = "#ffffa0";
    }
}

function savePageState() {
    stateData = {
        path: location.pathname,
        pageYOffset: window.pageYOffset
    };
    window.history.replaceState(stateData, null);
}

function expandToItem(targetItem) {
    var parent = targetItem.parentElement;
    if (parent.tagName == "A") {
        parent = parent.parentElement;
    }
    while (parent.tagName == "UL") {
        parent.classList.add("active");
        var li = parent.previousElementSibling;
        if (li) {
            if (li.tagName == "A") {
                li = li.children[0];
            }
            li.classList.add("caret-down");
        }                        
        parent = parent.parentElement;
    }
}

function amountToScroll(container, item) {
    var top = item.getBoundingClientRect().top - container.getBoundingClientRect().top;
    if (top < 0)
        return top;
    var bottom = item.getBoundingClientRect().bottom - container.getBoundingClientRect().bottom;
    if (bottom > 0)
        return bottom;
    return 0;
}

function navigateTo(path, forward, pageYOffset) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            // Extract page title
            var n = this.responseText.indexOf("\n");
            document.title = this.responseText.substr(0, n);
            article = document.getElementById("article");
            article.innerHTML = this.responseText.substr(n + 1);
            replaceLinks(article);

            // Scroll
            var anchorPos = path.indexOf("#");
            var pathToMatch = (anchorPos >= 0) ? path.substr(0, anchorPos) : path;
            if (forward && anchorPos >= 0) {
                location.hash = path.substr(anchorPos);
            } else {
                window.scrollTo(0, pageYOffset);
            }

            // Select appropriate TOC entry
            if (selected) {
                selected.classList.remove("selected");
                selected = null;
            }
            sidebar = document.querySelector(".sidebar");
            var list = sidebar.getElementsByTagName("li");
            for (var j = 0; j < list.length; j++) {
                var li = list[j];
                if (li.parentElement.getAttribute("href") == pathToMatch) {
                    selected = li;
                    li.classList.add("selected");
                    expandToItem(li);
                    var scrollAmount = amountToScroll(sidebar, li);
                    if (scrollAmount != 0) {
                        if (forward) {
                            var start = null;
                            var scrollFrom = sidebar.scrollTop;
                            var scrollTo = scrollFrom + scrollAmount;
                            requestAnimationFrame(function(timestamp) {
                                if (start === null) {
                                    start = timestamp;
                                } else {
                                    var f = (timestamp - start) / 100;
                                    if (f < 1) {
                                        sidebar.scrollTop = scrollTo * f + scrollFrom * (1 - f);
                                    } else {
                                        sidebar.scrollTop = scrollTo;
                                        return;
                                    }
                                }
                                requestAnimationFrame(arguments.callee);
                            });
                        } else {
                            sidebar.scrollTop = sidebar.scrollTop + scrollAmount;
                        }
                    }                    
                }
            }
            
            // Update history
            if (forward) {
                history.pushState(null, null, path);
                savePageState();
            }
        }
    };
    xhttp.open("GET", "/content?path=" + path, true);
    xhttp.send();            
}

function replaceLinks(root) {
    var list = root.getElementsByTagName("a");
    for (var i = 0; i < list.length; i++) {
        var a = list[i];
        var path = a.getAttribute("href");
        if (path.substr(0, 6) == "/docs/") {
            a.onclick = function() {
                savePageState();
                navigateTo(this.getAttribute("href"), true, 0);
                return false;
            }
        }
    }
}

function onEndTransition(evt) {
    this.removeEventListener('transitionend', onEndTransition);
    this.style.removeProperty("display");
    this.style.removeProperty("transition");
    this.style.removeProperty("height");
}

window.onload = function() { 
    highlight(location.hash.substr(1));

    var list = document.getElementsByClassName("caret");
    for (var i = 0; i < list.length; i++) {
        list[i].addEventListener("click", function() {
            var childList = this.nextElementSibling || this.parentElement.nextElementSibling;
            if (this.classList.contains("caret-down")) {
                // Collapse
                this.classList.remove("caret-down")
                childList.style.display = "block";
                childList.style.height = childList.scrollHeight + "px";
                childList.classList.remove("active");
                requestAnimationFrame(function() {
                    childList.style.transition = "height 0.15s ease-out";
                    requestAnimationFrame(function() {
                        childList.style.height = "0px";
                        childList.addEventListener('transitionend', onEndTransition);
                    });
                });
            } else {
                // Expand
                this.classList.add("caret-down");
                childList.style.display = "block";
                childList.style.transition = "height 0.15s ease-out";
                childList.style.height = childList.scrollHeight + "px";
                childList.classList.add("active");
                childList.addEventListener('transitionend', onEndTransition);
            }
        });
    }

    var sidebar = document.querySelector(".sidebar");
    replaceLinks(sidebar);
    replaceLinks(document.getElementById("article"));
    selected = sidebar.querySelector(".selected");
}

window.onhashchange = function() { 
    highlight(location.hash.substr(1));
}

window.onscroll = function() {
    savePageState();
}

window.addEventListener("popstate", function(evt) {
    if (evt.state) {
        navigateTo(evt.state.path, false, evt.state.pageYOffset);
    }
});
